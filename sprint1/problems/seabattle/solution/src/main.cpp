#ifdef WIN32
#include <sdkddkver.h>
#endif

#include "seabattle.h"

#include <atomic>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <iostream>
#include <optional>
#include <string>
#include <thread>
#include <string_view>

namespace net = boost::asio;
using net::ip::tcp;
using namespace std::literals;

void PrintFieldPair(const SeabattleField& left, const SeabattleField& right) {
    auto left_pad = "  "s;
    auto delimeter = "    "s;
    std::cout << left_pad;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << delimeter;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << std::endl;
    for (size_t i = 0; i < SeabattleField::field_size; ++i) {
        std::cout << left_pad;
        left.PrintLine(std::cout, i);
        std::cout << delimeter;
        right.PrintLine(std::cout, i);
        std::cout << std::endl;
    }
    std::cout << left_pad;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << delimeter;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << std::endl;
}

template <size_t sz>
static std::optional<std::string> ReadExact(tcp::socket& socket) {
    boost::array<char, sz> buf;
    boost::system::error_code ec;

    net::read(socket, net::buffer(buf), net::transfer_exactly(sz), ec);

    if (ec) {
        return std::nullopt;
    }

    return {{buf.data(), sz}};
}

static bool WriteExact(tcp::socket& socket, std::string_view data) {
    boost::system::error_code ec;

    net::write(socket, net::buffer(data), net::transfer_exactly(data.size()), ec);

    return !ec;
}

class SeabattleAgent {
public:
    SeabattleAgent(const SeabattleField& field)
        : my_field_(field) {
    }

    void StartGame(tcp::socket& socket, bool my_initiative) {
        PrintFields();
        if (my_initiative) {
            while (!IsGameEnded()) {
                ProcessYourTurn(socket);
                ProcessOpTurn(socket);
            }
        }
        else {
            while (!IsGameEnded()) {
                ProcessOpTurn(socket);
                ProcessYourTurn(socket);
            }
        }
    }

private:
    static std::optional<std::pair<int, int>> ParseMove(const std::string_view& sv) {
        if (sv.size() != 2) return std::nullopt;

        int p1 = sv[0] - 'A', p2 = sv[1] - '1';

        if (p1 < 0 || p1 > 8) return std::nullopt;
        if (p2 < 0 || p2 > 8) return std::nullopt;

        return {{p1, p2}};
    }

    static std::string MoveToString(std::pair<int, int> move) {
        char buff[] = {static_cast<char>(move.first) + 'A', static_cast<char>(move.second) + '1'};
        return {buff, 2};
    }

    void PrintFields() const {
        PrintFieldPair(my_field_, other_field_);
    }

    bool IsGameEnded() const {
        return my_field_.IsLoser() || other_field_.IsLoser();
    }

    std::pair<int, int> ProcessMove() const {
        std::optional<std::pair<int, int>> move;
        while (true) {
            std::cout << "Your turn: ";
            std::string str;
            std::cin >> str;
            std::cout << std::endl;
            move = ParseMove(str);
            if (move && other_field_(std::size_t(move->second), std::size_t(move->first)) == SeabattleField::State::UNKNOWN) {
                break;
            }
            else {
                std::cout << "Move not valid" << std::endl;
            }
        }
        return *move;
    }

    void ProcessShoot(SeabattleField::ShotResult res, std::pair<int, int> move, bool is_my_field) {
        switch (res) {
        case SeabattleField::ShotResult::HIT:
            std::cout << "Hit!" << std::endl;
            is_my_field ? my_field_.MarkHit(move.second, move.first) : other_field_.MarkHit(move.second, move.first);
            break;
        case SeabattleField::ShotResult::KILL:
            std::cout << "KILL!" << std::endl;
            is_my_field ? my_field_.MarkKill(move.second, move.first) : other_field_.MarkKill(move.second, move.first);
            break;
        case SeabattleField::ShotResult::MISS:
            std::cout << "Miss!" << std::endl;
            is_my_field ? my_field_.MarkMiss(move.second, move.first) : other_field_.MarkMiss(move.second, move.first);
            break;
        }
    }

    void ProcessYourTurn(tcp::socket& socket) {
        while (true) {
            if (other_field_.IsLoser()) {
                std::cout << "YOU WON!!!" << std::endl;
                return;
            }
            auto move = ProcessMove();
            WriteExact(socket, MoveToString(move));
            auto res = ReadExact<1>(socket);
            if (!res) {
                return;
            }
            auto parsed_result = SeabattleField::ShotResult(res.value().at(0));
            ProcessShoot(parsed_result, move, false);
            PrintFields();
            if (parsed_result == SeabattleField::ShotResult::MISS) {
                return;
            }
        }
    }

    void ProcessOpTurn(tcp::socket& socket) {
        while (true) {
            std::cout << "Waiting for opponent's turn..." << std::endl;
            auto op_move = ReadExact<2>(socket);
            std::pair<int, int> op_move_parsed;
            if (op_move) {
                op_move_parsed = *ParseMove(std::string_view(*op_move));
            }
            auto res = my_field_.Shoot(op_move_parsed.second, op_move_parsed.first);
            std::cout << "Shot to " << *op_move << std::endl;
            ProcessShoot(res, op_move_parsed, true);
            PrintFields();
            std::string str{ char(res) };
            WriteExact(socket, str);
            if ((res == SeabattleField::ShotResult::HIT || res == SeabattleField::ShotResult::KILL) && !IsGameEnded()) {
                //continue;
            }
            else if (my_field_.IsLoser()) {
                std::cout << "YOU LOSE!!!" << std::endl;
                return;
            }
            else {
                break;
            }
        }
    }

private:
    SeabattleField my_field_;
    SeabattleField other_field_;
};

void StartServer(const SeabattleField& field, unsigned short port) {
    SeabattleAgent agent(field);

    net::io_context io_context;

    tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), port));
    std::cout << "Waiting for connection..."sv << std::endl;

    boost::system::error_code ec;
    tcp::socket socket{ io_context };
    acceptor.accept(socket, ec);

    if (ec) {
        std::cout << "Can't accept connection"sv << std::endl;
        return;
    }

    agent.StartGame(socket, false);
};

void StartClient(const SeabattleField& field, const std::string& ip_str, unsigned short port) {
    SeabattleAgent agent(field);

    boost::system::error_code ec;
    auto endpoint = tcp::endpoint(net::ip::make_address(ip_str, ec), port);

    if (ec) {
        std::cout << "Wrong IP format"sv << std::endl;
        return;
    }

    net::io_context io_context;
    tcp::socket socket{ io_context };
    socket.connect(endpoint, ec);

    if (ec) {
        std::cout << "Can't connect to server"sv << std::endl;
        return;
    }

    agent.StartGame(socket, true);
};

int main(int argc, const char** argv) {
    if (argc != 3 && argc != 4) {
        std::cout << "Usage: program <seed> [<ip>] <port>" << std::endl;
        return 1;
    }

    std::mt19937 engine(std::stoi(argv[1]));
    SeabattleField fieldL = SeabattleField::GetRandomField(engine);

    if (argc == 3) {
        StartServer(fieldL, std::stoi(argv[2]));
    } else if (argc == 4) {
        StartClient(fieldL, argv[2], std::stoi(argv[3]));
    }
}

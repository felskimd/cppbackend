#include "sdk.h"
#include <boost/program_options.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/io_context.hpp>
#include <filesystem>
#include <iostream>
#include <optional>
#include <thread>

#include "json_loader.h"
#include "model_serialization.h"
#include "request_handler.h"
#include "stat_saver_impl.h"
#include "ticker.h"

using namespace std::literals;
namespace net = boost::asio;
namespace sys = boost::system;
namespace logging = boost::log;

BOOST_LOG_ATTRIBUTE_KEYWORD(additional_data, "AdditionalData", boost::json::value);

namespace {

struct Args {
    std::string config_path;
    std::string static_path;
    std::string state_path;
    int tick_time;
    bool randomize_spawn = false;
    bool no_auto_tick = true;
    int saving_period;
    bool auto_save = false;
    bool no_saving_file = false;
};

[[nodiscard]] std::optional<Args> ParseCommandLine(int argc, const char* const argv[]) {
    namespace po = boost::program_options;

    po::options_description desc{ "All options"s };

    Args args;
    desc.add_options()
        // Добавляем опцию --help и её короткую версию -h
        ("help,h", "produce help message")
        ("tick-period,t", po::value(&args.tick_time)->value_name("milliseconds"s), "set tick period")
        ("config-file,c", po::value(&args.config_path)->value_name("path"s), "set config file path")
        ("www-root,w", po::value(&args.static_path)->value_name("path"s), "set static files root")
        ("randomize-spawn-points", "spawn dogs at random positions")
        ("state-file", po::value(&args.state_path)->value_name("path"s), "set saving file path")
        ("save-state-period", po::value(&args.saving_period)->value_name("milliseconds"s), "set autosave period");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.contains("help"s)) {
        // Если был указан параметр --help, то выводим справку и возвращаем nullopt
        std::cout << desc;
        return std::nullopt;
    }

    // Проверяем наличие опций
    if (!vm.contains("config-file"s)) {
        throw std::runtime_error("No path to config"s);
    }
    if (!vm.contains("www-root"s)) {
        throw std::runtime_error("No path to static"s);
    }
    if (vm.contains("tick-period"s)) {
        args.no_auto_tick = false;
    }
    if (vm.contains("randomize-spawn-points")) {
        args.randomize_spawn = true;
    }
    if (!vm.contains("state-file"s)) {
        args.no_saving_file = true;
    }
    if (!args.no_saving_file && vm.contains("save-state-period"s)) {
        args.auto_save = true;
    }

    // С опциями программы всё в порядке, возвращаем структуру args
    return args;
}

// Запускает функцию fn на n потоках, включая текущий
template <typename Fn>
void RunWorkers(unsigned n, const Fn& fn) {
    n = std::max(1u, n);
    std::vector<std::jthread> workers;
    workers.reserve(n - 1);
    // Запускаем n-1 рабочих потоков, выполняющих функцию fn
    while (--n) {
        workers.emplace_back(fn);
    }
    fn();
}

void InitLogger() {
    logging::add_console_log(
        std::cout,
        logging::keywords::format = &http_handler::LoggingRequestHandler::Formatter,
        logging::keywords::auto_flush = true
    );

    logging::add_common_attributes();
}

}  // namespace

int main(int argc, const char* argv[]) {
    try {
        auto args = ParseCommandLine(argc, argv);
        if (!args) {
            return EXIT_SUCCESS;
        }

        InitLogger();

        // 1. Загружаем карту из файла и построить модель игры

        const char* db_url = std::getenv("GAME_DB_URL");
        if (!db_url) {
            db_url = "postgres://postgres:Mys3Cr3t@127.0.0.1:5432/postgres";
            //throw std::runtime_error("Can't get GAME_DB_URL from env");
        }

        const unsigned num_threads = std::thread::hardware_concurrency();
        auto shared_pool = std::make_shared<database::ConnectionPool>(num_threads, [db_url] {
            return std::make_shared<pqxx::connection>(db_url);
        });
        database::InitializeDB(shared_pool);
        auto stat_saver = std::make_shared<model::StatSaverImpl>(shared_pool);
        app::Application app{std::move(json_loader::LoadGame(args->config_path)), args->randomize_spawn, stat_saver};

        auto sl = std::make_shared<serialization::SerializingListener>(static_cast<unsigned>(args->saving_period), args->state_path, app);

        if (args->auto_save) {
            app.SetAppListener(sl);
        }
        
        if (!args->no_saving_file && std::filesystem::exists(args->state_path)) {
            serialization::Deserialize(args->state_path, app);
        }

        // 2. Инициализируем io_context
        net::io_context ioc(num_threads);

        // 3. Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc, &args, &app](const sys::error_code& ec, [[maybe_unused]] int signal_number) {
            if (!ec) {
                std::cout << "Signal "sv << signal_number << " received"sv << std::endl;
                ioc.stop();
            }
        });

        // 4. Создаём обработчик HTTP-запросов и связываем его с моделью игры
        auto stat_provider = std::make_shared<database::StatProvider>(shared_pool);
        auto handler = std::make_shared<http_handler::RequestHandler>(app, args->static_path.data(), ioc, args->no_auto_tick, stat_provider);
        http_handler::LoggingRequestHandler log_handler{handler};

        if (!args->no_auto_tick) {
            if (args->tick_time < 1) {
                throw std::runtime_error("Wrong tick time");
            }
            auto ticker = std::make_shared<ticker::Ticker>(handler->GetStrand(), std::chrono::milliseconds(args->tick_time),
                [&app](std::chrono::milliseconds delta) { 
                    app.Tick(delta.count()); 
                }
            );
            ticker->Start();
        }

        // 5. Запустить обработчик HTTP-запросов, делегируя их обработчику запросов
        const auto address = net::ip::make_address("0.0.0.0");
        constexpr net::ip::port_type port = 8080;
        http_server::ServeHttp(ioc, {address, port}, [&log_handler](auto&& req, auto&& send, const net::ip::address& address) {
            log_handler(std::forward<decltype(req)>(req), std::forward<decltype(send)>(send), address);
        });

        // Эта надпись сообщает тестам о том, что сервер запущен и готов обрабатывать запросы
        boost::json::value starting_data{ {"port"s, port}, {"address"s, address.to_string()} };
        BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, starting_data)
            << "server started"sv;

        // 6. Запускаем обработку асинхронных операций
        RunWorkers(std::max(1u, num_threads), [&ioc] {
            ioc.run();
        });

        if (!args->no_saving_file) {
            serialization::Serialize(args->state_path, app);
        }

        boost::json::value exiting_data{ {"code"s, 0} };
        BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, exiting_data)
            << "server exited"sv;

    } catch (const std::exception& ex) {
        std::cerr << ex.what() << std::endl;
        return EXIT_FAILURE;
    }
}

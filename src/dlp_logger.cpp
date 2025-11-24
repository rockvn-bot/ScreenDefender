#include "dlp_logger.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <filesystem>

static std::shared_ptr<spdlog::logger> g_logger;

void InitLogger()
{
    try
    {
        // Ensure directory exists
        std::filesystem::create_directories(DLP_LOG_DIRECTORY);

        std::string logFile = std::string(DLP_LOG_FILE);

        // Rotating file sink → 5 MB max, keep last 5 logs
        auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            logFile,
            MAX_FILE_SIZE,
            MAX_FILES
        );

#ifdef LOG_ENABLE_CONSOLE
        auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

        std::vector<spdlog::sink_ptr> sinks{ consoleSink, fileSink };
        g_logger = std::make_shared<spdlog::logger>("dlp_logger", sinks.begin(), sinks.end());
#else
        // File-only logging
        g_logger = std::make_shared<spdlog::logger>("dlp_logger", fileSink);
#endif

        g_logger->set_level(spdlog::level::debug);     // Capture all logs
        g_logger->flush_on(spdlog::level::info);       // Flush immediately for info+

        spdlog::set_default_logger(g_logger);

        g_logger->info("Logger initialized at path: {}", DLP_LOG_DIRECTORY);
    }
    catch (const spdlog::spdlog_ex& ex)
    {
        // Fallback silently to console only
        printf("[LOGGER ERROR] Failed to initialize logger: %s\n", ex.what());
    }
}

void LogInfo(const std::string& msg)
{
    if (g_logger) g_logger->info(msg);
}

void LogError(const std::string& msg)
{
    if (g_logger) g_logger->error(msg);
}

void LogDebug(const std::string& msg)
{
    if (g_logger) g_logger->debug(msg);
}

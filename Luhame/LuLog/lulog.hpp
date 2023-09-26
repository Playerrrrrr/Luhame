#pragma once
#include"spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include<atomic>
//设置两个日志，一个是CoreLog，一个是Log，前者是面向我的，后者是面向客户端程序员的

namespace Luhame {
	class LuLog {
	public:
		static auto GetCoreLogger() {
			if (!isInit)
				init();
			return coreLogger; 
		}
		static auto GetClientLogger() { 
			if (!isInit)
				init();
			return coreLogger; 
		}
	private:
		static void init() {
			spdlog::set_pattern("%^[%T] %n: %v%$");
			clientLogger = spdlog::stdout_color_mt("CLIENT");
			clientLogger->set_level(spdlog::level::trace);
			coreLogger = spdlog::stdout_color_mt("CORE");
			coreLogger->set_level(spdlog::level::trace);
			isInit = true;
		}
		inline static std::shared_ptr<spdlog::logger> coreLogger;
		inline static std::shared_ptr<spdlog::logger> clientLogger;
		inline static std::atomic<bool> isInit{false};
	};
	static const char* merge(const char* charStr1,const char* charStr2) {
		static thread_local char res[1024 * 2];
		strcpy_s(res, charStr1);  // 将第一个字符串拷贝到结果字符串中
		strcat_s(res, charStr2);  // 连接第二个字符串到结果字符串末尾
		return res;
	}
	static const char* merge_with_file_line(const char* file,int line,const char* mesg) {
		// "[at " __FILE__ " line:" "{}" "]: " x 
		static thread_local char res[1024 * 2];
		static thread_local char num[20];
		sprintf_s(num, "%d", line);
		strcpy_s(res, "[at ");
		strcat_s(res, file);
		strcat_s(res, " line: ");
		strcat_s(res, num);
		strcat_s(res, "]: ");
		strcat_s(res, mesg);
		return res;
	}
}

#define NULLLOG ""

//平凡log
#define LU_CORE_TRACE(...)    ::Luhame::LuLog::GetCoreLogger()->trace(__VA_ARGS__)
#define LU_CORE_INFO(...)     ::Luhame::LuLog::GetCoreLogger()->info(__VA_ARGS__)
#define LU_CORE_WARN(...)     ::Luhame::LuLog::GetCoreLogger()->warn(__VA_ARGS__)
#define LU_CORE_ERROR(...)    ::Luhame::LuLog::GetCoreLogger()->error(__VA_ARGS__)
#define LU_CORE_CRITICAL(...) ::Luhame::LuLog::GetCoreLogger()->critical(__VA_ARGS__)

//伴随文件和行号
//无参的时候需要在末尾加上NULLLOG
#define LU_CORE_TRACE_FL(x,...)    ::Luhame::LuLog::GetCoreLogger()->trace(  "[at " __FILE__ " line: " "{}" "]: " x ,__LINE__,__VA_ARGS__)
#define LU_CORE_INFO_FL(x,...)     ::Luhame::LuLog::GetCoreLogger()->info(  "[at " __FILE__ " line: " "{}" "]: " x, __LINE__,__VA_ARGS__)
#define LU_CORE_WARN_FL(x,...)     ::Luhame::LuLog::GetCoreLogger()->warn(  "[at " __FILE__ " line: " "{}" "]: " x, __LINE__,__VA_ARGS__)
#define LU_CORE_ERROR_FL(x,...)    ::Luhame::LuLog::GetCoreLogger()->error( "[at " __FILE__ " line: " "{}" "]: " x,__LINE__,__VA_ARGS__)
#define LU_CORE_CRITICAL_FL(x,...) ::Luhame::LuLog::GetCoreLogger()->critical(  "[at " __FILE__ " line:" "{}" "]: " x,__LINE__,__VA_ARGS__)

//非静态字符串,DS = dynamic string
#define LU_CORE_TRACE_DS(x)    ::Luhame::LuLog::GetCoreLogger()->trace(  ::Luhame::merge_with_file_line(__FILE__,__LINE__,x))
#define LU_CORE_INFO_DS(x)     ::Luhame::LuLog::GetCoreLogger()->info(  ::Luhame::merge_with_file_line(__FILE__,__LINE__,x))
#define LU_CORE_WARN_DS(x)     ::Luhame::LuLog::GetCoreLogger()->warn(  ::Luhame::merge_with_file_line(__FILE__,__LINE__,x))
#define LU_CORE_ERROR_DS(x)    ::Luhame::LuLog::GetCoreLogger()->error( ::Luhame::merge_with_file_line(__FILE__,__LINE__,x))
#define LU_CORE_CRITICAL_DS(x) ::Luhame::LuLog::GetCoreLogger()->critical( ::Luhame::merge_with_file_line(__FILE__,__LINE__,x))


#define LU_TRACE(...)    ::Luhame::LuLog::GetCoreLogger()->trace(__VA_ARGS__)
#define LU_INFO(...)     ::Luhame::LuLog::GetCoreLogger()->info(__VA_ARGS__)
#define LU_WARN(...)     ::Luhame::LuLog::GetCoreLogger()->warn(__VA_ARGS__)
#define LU_ERROR(...)    ::Luhame::LuLog::GetCoreLogger()->error(__VA_ARGS__)
#define LU_CRITICAL(...) ::Luhame::LuLog::GetCoreLogger()->critical(__VA_ARGS__)

#define LU_TRACE_FL(x,...)    ::Luhame::LuLog::GetClientLogger()->trace(  "[at " __FILE__ " line: " "{}" "]: " x ,__LINE__,__VA_ARGS__)
#define LU_INFO_FL(x,...)     ::Luhame::LuLog::GetClientLogger()->info(  "[at " __FILE__ " line: " "{}" "]: " x, __LINE__,__VA_ARGS__)
#define LU_WARN_FL(x,...)     ::Luhame::LuLog::GetClientLogger()->warn(  "[at " __FILE__ " line: " "{}" "]: " x, __LINE__,__VA_ARGS__)
#define LU_ERROR_FL(x,...)    ::Luhame::LuLog::GetClientLogger()->error( "[at " __FILE__ " line: " "{}" "]: " x,__LINE__,__VA_ARGS__)
#define LU_CRITICAL_FL(x,...) ::Luhame::LuLog::GetClientLogger()->critical(  "[at " __FILE__ " line: " "{}" "]: " x,__LINE__,__VA_ARGS__)


#define LU_TRACE_DS(x)    ::Luhame::LuLog::GetClientLogger()->trace(  ::Luhame::merge_with_file_line(__FILE__,__LINE__,x))
#define LU_INFO_DS(x)     ::Luhame::LuLog::GetClientLogger()->info(  ::Luhame::merge_with_file_line(__FILE__,__LINE__,x))
#define LU_WARN_DS(x)     ::Luhame::LuLog::GetClientLogger()->warn(  ::Luhame::merge_with_file_line(__FILE__,__LINE__,x))
#define LU_ERROR_DS(x)    ::Luhame::LuLog::GetClientLogger()->error( ::Luhame::merge_with_file_line(__FILE__,__LINE__,x))
#define LU_CRITICAL_DS(x) ::Luhame::LuLog::GetClientLogger()->critical( ::Luhame::merge_with_file_line(__FILE__,__LINE__,x))



#include <includes.hpp>


#ifndef CI_BUILD
#include <utils/io_utils.hpp>
#include <core/config.hpp>
#include <core/memory_allocator_static.hpp>
//#include <jsonrpccxx/server.hpp>

namespace {
	int test(Process& proc, int argc, const char* argv[]) {

		return tool::OK;
	}

	int testurl(Process& proc, int argc, const char* argv[]) {
		if (argc < 3) return tool::BAD_USAGE;
		// https://raw.githubusercontent.com/ate47/atian-cod-tools/main/release/version
		std::string out{};
		if (!utils::io::DownloadFile(argv[2], out)) {
			LOG_ERROR("Can't download {}", argv[2]);
			return tool::BASIC_ERROR;
		}
		LOG_INFO("{}", out);
		return tool::OK;
	}

	int cfgtest(Process& proc, int argc, const char* argv[]) {
		using namespace core::config;
		SyncConfig(true);

		LOG_INFO("file: {}", GetMainConfig().configFile.string());

		LOG_INFO("{}", GetInteger("test", 64));
		LOG_INFO("{}", GetBool("test2.bool.val", true));
		LOG_INFO("{}", GetInteger("test2.int", 64));
		LOG_INFO("{}", GetDouble("zqdzqd.dzqdzq.zdqzdh.hqzdz", 42.69));
		LOG_INFO("{}", GetString("str", "dzqzqdzdqzdq"));

		LOG_INFO("{}", GetString("ui.injector.path"));

		//main["test3"] = 42;
		//
		//LOG_INFO("Key:");
		//for (auto& [key, val] : main.GetObj()) {
		//	LOG_INFO("- {}", key.GetString());
		//}

		SetString("zdzdq", "qzddqzqzdqzd");

		SaveConfig();

		return tool::OK;
	}
	int memalloctest(Process& proc, int argc, const char* argv[]) {
		core::memory_allocator::MemoryAllocatorStatic<0x2000> alloc{};

		void* ptr1 = alloc.Alloc(0x20);
		LOG_INFO("{}", ptr1);
		void* ptr2 = alloc.Alloc(0x10);
		LOG_INFO("{}", ptr2);
		void* ptr3 = alloc.Alloc(0x18);
		LOG_INFO("{}", ptr3);

		alloc.Free(ptr1);
		alloc.Free(ptr3);
		void* ptr4 = alloc.Alloc(0x30);
		LOG_INFO("{}", ptr4);
		void* ptr5 = alloc.Alloc(0x10);
		LOG_INFO("{}", ptr5);


		return tool::OK;
	}
	
	int strtouint64(int argc, const char* argv[]) {

		char buff[sizeof(uint64_t) + 1];
		for (size_t i = 2; i < argc; i++) {
			const char* arg{ argv[i] };

			size_t len{ std::strlen(arg) };

			if (len > 8) {
				LOG_ERROR("String {} too long, can't convert to uint64", arg);
				continue;
			}

			std::memset(buff, 0, sizeof(buff));
			std::memcpy(buff, arg, len);
			LOG_INFO("{} = 0x{:x}", arg, *(uint64_t*)&buff[0]);
		}

		return tool::OK;
	}
}

ADD_TOOL(test, "dev", "", "Tests", nullptr, test);
ADD_TOOL(strtouint64, "common", " (str)*", "Convert string to number", strtouint64);
ADD_TOOL(memalloctest, "dev", "", "Tests", nullptr, memalloctest);
ADD_TOOL(wget, "dev", " [url]", "Tests", nullptr, testurl);
ADD_TOOL(cfgtest, "dev", "", "", nullptr, cfgtest);
#endif
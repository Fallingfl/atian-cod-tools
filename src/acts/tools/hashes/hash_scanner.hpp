#pragma once

namespace tool::hash::scanner {
	void ScanHashes(const std::vector<std::filesystem::path>& files, std::unordered_set<uint64_t>& hashes);


}
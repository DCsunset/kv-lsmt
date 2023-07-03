#ifndef ENGINE_RACE_SSTABLE_H_
#define ENGINE_RACE_SSTABLE_H_

#include "../include/engine.h"
#include <map>
#include <vector>
#include <fstream>

namespace polar_race {
	class SSTable;

	SSTable mergeTable(const SSTable &table1, const SSTable &table2);

	// Sorted String Table
	class SSTable {
		public:
			RetCode Write(const std::string& key,
					const std::string& value);
			RetCode Read(const std::string& key,
					std::string* value);
			// After load, the index becomes offset
			RetCode LoadIndex(const std::string &filename);
			// Use the offset in index
			RetCode ReadFromFile(const std::string &filename, const std::string& key,
					std::string* value);

			std::ofstream indexOf;
			std::ofstream dataOf;
			bool fileOpen = true;

			auto size() {
				return index_.size();
			}

		private:
			// <key, index/offset>
			std::map<std::string, size_t> index_;
			std::vector<std::string> data_;

			friend SSTable mergeTable(const SSTable &table1, const SSTable &table2);
	};
} // namespace namepolar_race

#endif

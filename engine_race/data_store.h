#ifndef ENGINE_RACE_DATA_STORE_H_
#define ENGINE_RACE_DATA_STORE_H_

#include "sstable.h"
#include <list>
#include <fstream>
#include <shared_mutex>

namespace polar_race {
	// Store on disk
	class DataStore {
		public:
			explicit DataStore(const std::string &dir)
				: dir_(dir) { load(); }

			RetCode Write(const std::string& key,
					const std::string& value);
			RetCode Read(const std::string& key,
					std::string* value);

		private:
			std::string dir_;
			// Current table is the last table in level 0
			std::vector<SSTable> tables_;
			mutable std::shared_mutex mutex_;

			// Load from disk
			void load();
			// Save to disk
			void save();

			void newTable();

			std::string indexFilename(int num) {
				return dir_ + "/0-" + std::to_string(num) + ".index";
			}
			std::string dataFilename(int num) {
				return dir_ + "/0-" + std::to_string(num) + ".data";
			}
	};
}

#endif

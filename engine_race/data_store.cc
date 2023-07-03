#include "data_store.h"
#include <filesystem>
#include <cstdio>
#include <set>
#include <fstream>
#include <iostream>
#include <mutex>

namespace fs = std::filesystem;

struct FileCompare {
	bool operator () (const std::string &lhs, const std::string &rhs) const {
		return std::stoi(lhs.substr(2)) < std::stoi(rhs.substr(2));
	}
};

namespace polar_race {
	void DataStore::load() {
		fs::create_directories(dir_);
		std::set<std::string, FileCompare> ps;
		for (auto& p: fs::directory_iterator(dir_)) {
			if (p.path().extension() == ".index")
				ps.insert(p.path().stem());
		}
		tables_.reserve(ps.size());
		for (auto &p : ps) {
			tables_.emplace_back();
			tables_.back().LoadIndex(dir_ + '/' + p + ".index");
		}
		newTable();
	}

	void DataStore::save() {
		if (tables_.size()) {
			tables_.emplace_back();
			tables_.back().LoadIndex(indexFilename(tables_.size()-1));
		}
	}

	void DataStore::newTable() {
		// TODO: Save index table to disk if full
		tables_.emplace_back();
		tables_.back().indexOf.open(indexFilename(tables_.size()-1), std::ios_base::binary);
		// Data file
		tables_.back().dataOf.open(dataFilename(tables_.size()-1), std::ios_base::binary);
	}

	RetCode DataStore::Write(const std::string& key,
		const std::string& value) {
		std::unique_lock lock(mutex_);
		// std::cout << "Write: " << key << ", " << value << std::endl;

		tables_.back().Write(key, value);
		if (tables_.back().size() >= 1000000) {
			save();
			newTable();
		}
		return kSucc;
	}

	RetCode DataStore::Read(const std::string& key,
		std::string* value) {
		std::shared_lock lock(mutex_);
		// std::cout << "Read: " << key << ", " << value << std::endl;

		if (tables_.back().Read(key, value) == kSucc)
			return kSucc;
		
		for (int i = tables_.size() - 2; i >= 0; --i) {
			if (tables_[i].ReadFromFile(dataFilename(i), key, value) == kSucc)
				return kSucc;
		}
		return kNotFound;
	}
}

#include "sstable.h"

namespace polar_race {
	RetCode SSTable::LoadIndex(const std::string &filename) {
		std::ifstream fin(filename, std::ios_base::binary);
		std::string key;
		size_t offset;
		size_t len;
		while (!fin.eof()) {
			fin.read((char *)&len, 8);
			key.resize(len);
			fin.read((char *)key.data(), len);
			fin.read((char *)&offset, 8);
			index_[key] = offset;
		}
		return kSucc;
	}

	RetCode SSTable::Read(const std::string& key, std::string* value) {
		if (!index_.count(key))
			return kNotFound;
		*value = data_[index_[key]];
		return kSucc;
	}

	RetCode SSTable::ReadFromFile(const std::string &filename, const std::string& key, std::string* value) {
		if (!index_.count(key))
			return kNotFound;
		
		std::ifstream fin(filename, std::ios_base::binary);
		// Catch error (bad write)
		fin.exceptions(std::ios::failbit | std::ios::badbit);
		try {
			fin.seekg(index_[key]);
			size_t len;
			fin.read((char *)&len, 8);
			value->resize(len);
			fin.read((char *)value->data(), len);
			return kSucc;
		}
		catch (std::exception &err) {
			// Unsuccessful write
			return kNotFound;
		}
	}

	RetCode SSTable::Write(const std::string& key, const std::string& value) {
		if (!index_.count(key)) {
			index_[key] = data_.size();
			data_.emplace_back();
		}
		data_[index_[key]] = value;

		// Save to file
		size_t len = key.size();
		indexOf.write((const char *)&len, 8);
		indexOf.write(key.data(), len);
		size_t offset = dataOf.tellp();
		indexOf.write((const char *)&offset, 8); // Save offset
		indexOf.flush();

		len = value.size();
		dataOf.write((const char *)&len, 8);
		dataOf.write(value.data(), len);
		dataOf.flush();
		return kSucc;
	}

	// Table 1 is created after table 2
	SSTable mergeTable(const SSTable &table1, const SSTable &table2) {
		SSTable result;
		// Disable file when merging
		result.fileOpen = false;
		auto it1 = table1.index_.begin(),
			it2 = table2.index_.begin();
		while (it1 != table1.index_.end()
			&& it2 != table2.index_.end()) {
			if (it1->first == it2->first
				|| it1->first < it2->first) {
				result.Write(it1->first, table1.data_[it1->second]);
				++it1;
			}
			else {
				result.Write(it2->first, table2.data_[it2->second]);
				++it2;
			}
		}
		while (it1 != table1.index_.end()) {
			result.Write(it1->first, table1.data_[it1->second]);
			++it1;
		}
		while (it2 != table2.index_.end()) {
			result.Write(it2->first, table2.data_[it2->second]);
			++it2;
		}

		return result;
	}
} // namespace name polar_race

#pragma once
#include <algorithm>
#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

template <typename T>
concept Serializable = requires(nlohmann::json j, T a) {
    { j.get<T>() } -> std::same_as<T>;
    { nlohmann::json(a) } -> std::same_as<nlohmann::json>;
};

template <Serializable T> class ConfigRegistry
{
  public:
    ConfigRegistry(const std::string &path, const std::string &tableName = "") : _filePath(path), _tableName(tableName)
    {
        load(); // optional preload
    }

    bool save(const T &obj) const
    {
        std::ofstream out(_filePath);
        if (!out)
        {
            // MessageBox(NULL, std::format("Error occurred trying to save {}", _filePath).c_str(), "Save Error", NULL);
            return false;
        }

        nlohmann::json j;
        if (_tableName.empty())
        {
            j = obj;
        }
        else
        {
            j[_tableName] = obj;
        }

        out << j.dump(4);
        return true;
    }

    std::optional<T> load()
    {
        std::ifstream in(_filePath);
        if (!in)
        {
            return std::nullopt;
        }

        nlohmann::json j;
        in >> j;

        try
        {
            if (_tableName.empty())
            {
                return j.get<T>();
            }
            else if (j.contains(_tableName))
            {
                return j[_tableName].get<T>();
            }
        }
        catch (...)
        {
            //
        }

        return std::nullopt;
    }

  private:
    std::string _filePath;
    std::string _tableName;
};

template <Serializable T> class TableRegistry
{
  public:
    TableRegistry(const std::string &path) : _filePath(path)
    {
        load();
    }

    void addToTable(const std::string &key, const T &entry)
    {
        if (_tables.find(key) == _tables.end())
            _tableNames.push_back(key);
        _tables[key].push_back(entry);
    }

    std::vector<T> getTable(const std::string &key) const
    {
        auto it = _tables.find(key);
        return (it != _tables.end()) ? it->second : std::vector<T>{};
    }

    std::optional<std::reference_wrapper<std::vector<T>>> getTableRef(const std::string &key)
    {
        auto it = _tables.find(key);
        if (it != _tables.end())
        {
            return std::ref(it->second);
        }
        else
        {
            return std::nullopt;
        }
    }

    void clearTable(const std::string &key)
    {
        if (auto it = _tables.find(key); it != _tables.end())
            it->second.clear();
    }

    void removeTable(const std::string &key)
    {
        if (_tables.erase(key))
            _tableNames.erase(std::remove(_tableNames.begin(), _tableNames.end(), key), _tableNames.end());
    }

    void removeByID(const std::string &id)
    {
        for (auto &[key, entries] : _tables)
        {
            entries.erase(std::remove_if(entries.begin(), entries.end(),
                                         [&id](const T &entry) { return const_cast<T &>(entry).toString() == id; }),
                          entries.end());
        }
    }

    void updateByID(const std::string &id, const T &updatedEntry)
    {
        for (auto &[key, entries] : _tables)
        {
            for (auto &entry : entries)
            {
                if (entry.toString() == id)
                {
                    entry = updatedEntry;
                    return;
                }
            }
        }
        addToTable(id, updatedEntry);
    }

    std::vector<T> findMatch(const std::function<bool(const T &)> &predicate)
    {
        std::vector<T> matches;
        for (const auto &[key, entries] : _tables)
        {
            for (const auto &entry : entries)
            {
                if (predicate(entry))
                {
                    matches.push_back(entry);
                }
            }
        }

        return matches;
    }

    std::unordered_map<std::string, std::vector<T>> &tables()
    {
        return _tables;
    }
    const std::vector<std::string> &tableNames()
    {
        return _tableNames;
    }

    bool save()
    {
        nlohmann::json j;
        for (const auto &[key, entries] : _tables)
        {
            j[key] = entries;
        }

        std::ofstream out(_filePath);
        if (!out)
        {
            // MessageBox(NULL, std::format("Error occured trying to save {}", _filePath).c_str(), "Save Error", NULL);
            return false;
        }
        out << j.dump(4);
        return true;
    }

    bool load()
    {
        if (!std::filesystem::exists(_filePath))
        {
            std::ofstream out(_filePath);
            if (!out)
            {
                return false;
            }
            out << "{}";
            out.close();
        }

        std::ifstream in(_filePath);
        if (!in)
        {
            return false;
        }

        nlohmann::json j;
        in >> j;

        _tables.clear();
        _tableNames.clear();

        for (auto it = j.begin(); it != j.end(); ++it)
        {
            _tables[it.key()] = it.value().get<std::vector<T>>();
            _tableNames.push_back(it.key());
        }
        return true;
    }

  private:
    std::string _filePath;
    std::unordered_map<std::string, std::vector<T>> _tables;
    std::vector<std::string> _tableNames;
};

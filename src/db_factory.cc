#include "db_factory.h"
#include "db_wrapper.h"

namespace benchmark {


std::map<std::string, DBFactory::DBCreator> &DBFactory::Registry() {
  static std::map<std::string, DBCreator> registry;
  return registry;
}

bool DBFactory::RegisterDB(std::string db_name, DBCreator db_creator) {
  Registry()[db_name] = db_creator;
  return true;
}

DB *DBFactory::CreateDB(utils::Properties *props, Measurements *measurements) {
  DB *db = CreateRawDB(props);
  if (db != nullptr) {
    return new DBWrapper(db, measurements);
  }
  return nullptr;
}

MemcacheWrapper *DBFactory::CreateMemcache(utils::Properties *props, Measurements *measurements) {
  DB *db = CreateRawDB(props);
  if (db != nullptr) {
    return new MemcacheWrapper(db, measurements);
  }
  return nullptr;
}

DB *DBFactory::CreateRawDB(utils::Properties *props) {
  std::string db_name = props->GetProperty("dbname", "test");
  DB *db = nullptr;
  std::map<std::string, DBCreator> &registry = Registry();
  if (registry.find(db_name) != registry.end()) {
    db = (*registry[db_name])();
    db->SetProps(props);
    db->Init();
  }
  return db;
}

} // benchmark

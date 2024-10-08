#pragma once
#include <map>
#include <vector>
#include "ConfigMap.hpp"
#include "ConfigAtom.hpp"

namespace configmaps
{
    class ConfigAtom;
    class ConfigItem;
    class ConfigVector;
    
    /**
     * @brief Represents a schema
     * Allowed schema types & constraints:
     *  Types:
     *      - integer (for integers)
     *      - number (for floating points and integers)
     *      - boolean (for boolean values true/false)
     *      - object (for maps)
     *      - array (for lists)
     * Constraints: 
     *      - minimum (set minimum value for integer or number)
     *      - maximum (set maximum value for integer or number)
     */
    class ConfigSchema
    {
    public:
        /**
         * @brief Construct a new ConfigSchema object by 
         * ConfigMap of schema structure 
         */
        explicit ConfigSchema(const ConfigMap &schema);
        ConfigSchema() ;
        
         /**
         * @brief Validates a ConfigMap whether its respecting this schema or not
         * 
         * @param config: ConfigMap config data
         * @return true if the config respects this schema, otherwise false.
         */
        bool validate(ConfigMap &config);

    private:
        ConfigMap m_schema;
        static const std::map<std::string, std::vector<ConfigAtom::ItemType>> SCHEMA_ATOM_TYPES;
        
    private:
        bool has_corresponding_type(ConfigItem &config_item, const std::string &type);

        bool is_known_type(const std::string &type);

        bool validate_keys(ConfigMap &config, ConfigMap& schema);

        bool validate_types(ConfigMap &config, ConfigMap& schema);

        bool validate_constraints(ConfigMap& config_item, const std::string& key, ConfigMap& schema);
    };
}

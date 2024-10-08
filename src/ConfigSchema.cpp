#include "ConfigSchema.hpp"
#include "ConfigAtom.hpp"
#include "ConfigItem.hpp"
#include "ConfigVector.hpp"
#include "ConfigMap.hpp"

using namespace configmaps;

const std::map<std::string, std::vector<configmaps::ConfigAtom::ItemType>> ConfigSchema::SCHEMA_ATOM_TYPES = {
    {"integer", {ConfigAtom::INT_TYPE, ConfigAtom::UINT_TYPE, ConfigAtom::ULONG_TYPE}},
    {"string", {ConfigAtom::STRING_TYPE}},
    {"number", {ConfigAtom::DOUBLE_TYPE, ConfigAtom::INT_TYPE, ConfigAtom::UINT_TYPE, ConfigAtom::ULONG_TYPE}},
    {"boolean", {ConfigAtom::BOOL_TYPE}},
};

ConfigSchema::ConfigSchema(const ConfigMap &schema) : m_schema(schema) {}
ConfigSchema::ConfigSchema() {}

bool ConfigSchema::validate(ConfigMap &config)
{
    // Check if we have a non-empty config first
    if (config.empty())
        return false;

    // Validate mandatory keys
    if (not validate_keys(config, m_schema))
        return false;

    // Validate value types
    if (not validate_types(config, m_schema))
        return false;

    return true;
}

bool ConfigSchema::has_corresponding_type(ConfigItem &config_item, const std::string &type)
{
    if (type == "object")
        return config_item.isMap();
    if (type == "array")
        return config_item.isVector();
    for(auto atomic_type : SCHEMA_ATOM_TYPES.at(type))
    {
        if(config_item.getOrCreateAtom()->testType(atomic_type))
        {
            return true;
        }
    }

    return false;
}

bool ConfigSchema::is_known_type(const std::string &type)
{
    if (SCHEMA_ATOM_TYPES.count(type))
        return true;
    return type == "object" or type == "array";
}

bool ConfigSchema::validate_keys(ConfigMap &config, ConfigMap &schema)
{
    for (auto const &[key, value] : schema)
    {
        // Check if required keys in schema exist in our config
        if (value.hasKey("required"))
        {
            if ((bool)value["required"] == true)
            {
                // there is a required field with a true value,
                // that means, schema 'key' MUST exist in 'config'
                if (not config.hasKey(key))
                {
                    std::cerr << "ConfigSchema::validate_keys: Missing required field \"" << key << "\" from config" << std::endl;
                    return false;
                }
            }
        }
        else
        {
            // If there is no required field, it's by default false (not required)
            // So if it doesn't exist in the config, there is no need to validate it
            if (not config.hasKey(key))
                continue;
        }
        // Take the opportunity to validate schema keys
        // Check if we have a type field in schema, its mandatory..
        if (not value.hasKey("type"))
        {
            std::cerr << "ConfigSchema::validate_keys: Missing schema type field for \"" << key << "\"" << std::endl;
            return false;
        }
        // If the item is an object, validate it recursively
        if (value["type"] == "object")
        {
            if (not config[key].isMap())
            {
                std::cerr << "ConfigSchema::validate_keys: Expected \"" << key << "\" to be an object" << std::endl;
                return false;
            }
            if (not value.hasKey("properties"))
            {
                std::cerr << "ConfigSchema::validate_keys: Expected \"properties\" field in object \"" << key << "\"" << std::endl;
                return false;
            }
            ConfigMap sub_schema = value["properties"];
            if (not validate_keys(config[key], sub_schema))
            {
                return false;
            }
        }
        else if (value["type"] == "array") // If the item is an array of objects, validate its elements
        {
            if (not config[key].isVector())
            {
                std::cerr << "ConfigSchema::validate_keys: Expected \"" << key << "\" to be an array" << std::endl;
                return false;
            }
            if (not value.hasKey("contains"))
            {
                std::cerr << "ConfigSchema::validate_keys: Expected array \"" << key << "\" to have \"contains\" field" << std::endl;
                return false;
            }
            if (not value["contains"].hasKey("type"))
            {
                std::cerr << "ConfigSchema::validate_keys: Expected schema \"type\" field in \"contains\" for array \"" << key << "\"" << std::endl;
                return false;
            }
            // If its an array containing object elements, validate each object
            if (value["contains"]["type"] == "object")
            {
                for (ConfigItem &obj : config[key])
                {
                    if (not value["contains"].hasKey("properties"))
                    {
                        std::cerr << "ConfigSchema::validate_keys: Expected \"properties\" field in object \"" << key << "\"" << std::endl;
                        return false;
                    }
                    ConfigMap sub_schema = value["contains"]["properties"];
                    if (not validate_keys(obj, sub_schema))
                    {
                        return false;
                    }
                }
            }
        }
    }
    return true;
}

bool ConfigSchema::validate_types(ConfigMap &config, ConfigMap &schema)
{
    for (auto const &[key, value] : schema)
    {
        if (not config.hasKey(key))
            continue; // A not required field, doesn't seem to exist. skip it.

        // Check first if the desired schema type is known
        if (not is_known_type(value["type"]))
        {
            std::cerr << "ConfigSchema::validate_types: Invalid schema type " << (std::string)value["type"] << " in \"" << key << '\"' << std::endl;
            return false;
        }
        // Check if the type defined in the schema is matching the type we have in config
        if (not has_corresponding_type(config[key], value["type"]))
        {
            std::cerr << "ConfigSchema::validate_types: Invalid value for \"" << key << "\", expected \"" << (std::string)value["type"] << '\"' << std::endl;
            return false;
        }
        // Check constraints
        if (not validate_constraints(config, key, value))
        {
            return false;
        }
        // Validate sub objects (recursively) obj{obj{}...}
        if (config[key].isMap())
        {
            if (not validate_types(config[key], value["properties"]))
                return false;
        }
        // Validate sub objects within the array if any [obj{}, obj{}, ...]
        else if (config[key].isVector())
        {
            for (ConfigItem &o : config[key])
            {
                if (value["contains"]["type"] == "object")
                {
                    ConfigMap sub_schema = value["contains"]["properties"];
                    if (not validate_types(o, sub_schema))
                        return false;
                }
                else
                {
                    if (not has_corresponding_type(o, value["contains"]["type"]))
                    {
                        std::cerr << "ConfigSchema::validate_types: Invalid type for \"" << key << '\"' << std::endl;
                        return false;
                    }
                }
            }
        }
    }
    return true;
}

bool ConfigSchema::validate_constraints(ConfigMap &config, const std::string &key, ConfigMap &schema)
{
    // minimum and maximum:
    if (schema.hasKey("minimum") or schema.hasKey("maximum"))
    {
        // Check atom constraints, floating points and integers
        if (schema["type"] != "number" and schema["type"] != "integer")
        {
            std::cerr << "ConfigSchema::validate_constraints: Invalid minimum,maximum schema in non-atom type in \"" << key << '\"' << std::endl;
            return false;
        }

        // Handle minimum constrains
        double minimum, maximum;
        if (schema.hasKey("minimum"))
            minimum = schema["minimum"].getOrCreateAtom()->testType(ConfigAtom::DOUBLE_TYPE) ? (double)schema["minimum"] : static_cast<double>((int)(schema["minimum"]));
        if (schema.hasKey("maximum"))
            maximum = schema["maximum"].getOrCreateAtom()->testType(ConfigAtom::DOUBLE_TYPE) ? (double)schema["maximum"] : static_cast<double>((int)(schema["maximum"]));

        // Check if the config item's value is in range [minimum, maximum]
        double value = config[key].getOrCreateAtom()->testType(ConfigAtom::DOUBLE_TYPE) ? (double)config[key] : static_cast<double>((int)(config[key]));
        if (schema.hasKey("minimum") and schema.hasKey("maximum"))
        {
            // Check if minimum is less than maximum
            if (minimum > maximum)
            {
                std::cerr << "ConfigSchema::validate_constraints: minimum value is greater than the maximum value in \"" << key << '\"' << std::endl;
                return false;
            }
            if (value < minimum or value > maximum)
            {
                std::cerr << "ConfigSchema::validate_constraints: value " << value << " is out of range [" << minimum << ", " << maximum << "] in \"" << key << '\"' << std::endl;
                return false;
            }
        }
        else if (schema.hasKey("minimum") and not schema.hasKey("maximum"))
        {
            if (value < minimum)
            {
                std::cerr << "ConfigSchema::validate_constraints: value " << value << " is out of range [" << minimum << ", " << (schema["maximum"].getOrCreateAtom()->testType(ConfigAtom::DOUBLE_TYPE) ? std::numeric_limits<double>::max() : std::numeric_limits<int>::max()) << "] in \"" << key << '\"' << std::endl;
                return false;
            }
        }
        else if (schema.hasKey("maximum") and not schema.hasKey("minimum"))
        {
            if (value > maximum)
            {
                std::cerr << "ConfigSchema::validate_constraints: value " << value << " is out of range [" << (schema["minimum"].getOrCreateAtom()->testType(ConfigAtom::DOUBLE_TYPE) ? std::numeric_limits<double>::min() : std::numeric_limits<int>::min()) << ", " << maximum << "] in \"" << key << '\"' << std::endl;
                return false;
            }
        }
    }
    
    return true;
}
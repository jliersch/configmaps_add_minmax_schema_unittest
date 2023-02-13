Documentation can be found at [doc](doc/configmaps.md).

In the restructured branch the internal representation is refactored. Instead
of using the map/vector/map/vector principle we now have a map of ConfigItems
and a ConfigItem is of type map, vector, or atom. The api is mainly kept the
same. Main difference is that the children property is removed. The master api
is also adapted to allow code that compiles against the restructured and the
"old" master branch. Changes that are needed to compile against both branches
are listed below:


Remove use of the children property:

```
   map["foo"][0].children["blub"] = "test";
   ->  map["foo"]["blub"] = "test";

   it = map["foo"][0].children.begin();
   ->  it = map["foo"].beginMap();

   The same for end() and find().

   A hasKey() function is added:
   map.hasKey("foo");          // for map["foo"]
   map["foo"].hasKey("blub");  // for map["foo"]["blub"]
```

Remove use of getInt(), getULong(), etc. methods:

```
   map["foo"][0].getInt();
   ->  map["foo"];
   map["foo"][0].getStirng();
   ->  (std::string)map["foo"];
```

Do not use the first vector element to access the first item:

```
   int i = map["foo"][0];
   ->  int i = map["foo"]
```

Operators provide pointers to subelements:

```
   ConfigMap &m = map["foo"][0].children;
   -> ConfigMap &m = map["foo"];

   ConfigMap *m = &(map["foo"][0].children);
   -> ConfigMap *m = map["foo"];

   The same for ConfigVector and ConfigItem;
```

To get the size of a submap:

```
   map["foo"][0].children.size();
   -> ((ConfigMap&)map["foo"]).size();
   The cast is only needed to be compatible to the olde implementation.
```

Don't use the ConfigItem constructor:

```
   map["foo"].push_back(ConfigItem("blub"));
   ->  map["foo"].push_back("blub");

   map["foo"] = ConfigItem("blub");
   ->  map["foo"] = "blub";
```


Assign value to std::string is ambiguous:

```
   std::string s = map["foo"];  // ok because uses the std::string constructor
   s = map["foo"]; // is ambiguous due to operator=(std::string) and operator=(char)

   alternative:
   s << map["foo"];
   
   you can use the streaming operator as general configmap assign operator:
   double d;
   d << map["some_value"];
   map["another_value"] << 3.14;
```

#### CMAKE 

To use this library from a CMake project, it should be locatable directly with `find_package()` and the namespaced imported target from the generated package configuration should be used:

```cmake
# CMakeLists.txt
find_package(configmaps REQUIRED)
...
add_library(foo ...)
...
target_link_libraries(foo PRIVATE configmaps::configmaps)
```
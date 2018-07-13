#ifndef API_FUZZER_HPP
#define API_FUZZER_HPP

#include <functional>
#include <iostream>
#include <iterator>
#include <string>
#include <sstream>
#include <set>
#include <tuple>
#include <vector>

#include "isl-noexceptions.h"
#include "fmt/format.h"
#include "yaml-cpp/yaml.h"

enum ApiTarget {
    ISL,
};

class ApiType;
class ApiFunc;
class ApiObject;

std::string getStringWithDelims(std::vector<std::string>, char);
template<typename T> std::string makeArgString(std::vector<T>);
template<typename T> T getRandomVectorElem(std::vector<T>&);
template<typename T> std::vector<const ApiObject*> filterObjList
    (std::vector<const ApiObject*>, bool (ApiObject::*)(T) const, T);

class ApiType {
    const std::string name;
    const bool singleton;

    public:
        ApiType(std::string _name, bool _singleton = false) : name(_name),
            singleton(_singleton) {};

        std::string getTypeStr() const;
        bool isType(const ApiType*) const;
        bool isSingleton() const;
        bool hasName(std::string) const;
        static ApiType getVoid();

        std::string toStr() const;

        inline bool operator<(const ApiType* other) const {
            return this->getTypeStr() < other->getTypeStr();
        }
};

class ApiObject {
    const unsigned int id;
    const std::string name;
    const ApiType* type;

    public:
        ApiObject(std::string _name, unsigned int _id, const ApiType* _type) :
            id(_id), name(_name), type(_type) {};
        std::string toStr() const;
        std::string toStrWithType() const;
        const ApiType* getType() const;
        bool hasType(const ApiType*) const;
};

class ApiFunc {
    const std::string name;
    const ApiType* member_type;
    const ApiType* return_type;
    const std::vector<const ApiType*> param_types;
    const std::vector<std::string> conditions;

    public:
        ApiFunc(std::string _name, const ApiType* _member_type, const ApiType* _return_type,
            std::vector<const ApiType*> _param_types,
            std::vector<std::string> _conditions) :
            name(_name), member_type(_member_type), return_type(_return_type),
            param_types(_param_types), conditions(_conditions) {};

        std::string getName() const;
        std::vector<const ApiType*> getParamTypes() const;
        const ApiType* getParamType(const unsigned int) const;
        unsigned int getParamCount() const;
        const ApiType* getMemberType() const;
        const ApiType* getReturnType() const;

        bool hasMemberType(const ApiType*) const;
        bool hasReturnType(const ApiType*) const;
        bool hasName(std::string) const;
        bool hasParamTypes(std::vector<const ApiType*>) const;

        bool checkArgs(std::vector<const ApiObject*>) const;
        std::string printSignature() const;
        std::string printInvocation(std::vector<const ApiObject*>) const;

        inline bool operator<(const ApiFunc& other) const {
            return this->printSignature() < other.printSignature();
        }
};

class ApiFuzzer {
    private:
        std::set<const ApiType*> types;
        std::set<const ApiFunc*> funcs;
        std::vector<const ApiObject*> objs;
        std::vector<std::string> instrs;
        unsigned int next_obj_id;

        virtual const ApiObject* generateObject(const ApiType*) = 0;

    public:
        ApiFuzzer(): next_obj_id(0), instrs(std::vector<std::string>()),
            objs(std::vector<const ApiObject*>()), types(std::set<const ApiType*>()),
            funcs(std::set<const ApiFunc*>()) {};

        std::vector<std::string> getInstrList();
        std::vector<const ApiObject*> getObjList();
        std::set<const ApiFunc*> getFuncList();
        std::set<const ApiType*> getTypeList();
        unsigned int getNextID();

        bool hasTypeName(std::string);
        bool hasFuncName(std::string);

        std::vector<const ApiObject*> filterObjByType(const ApiType*);
        std::vector<const ApiObject*> filterObjByType(std::string);
        const ApiType* getTypeByName(std::string);
        template<typename T> std::vector<const ApiObject*> filterObjs(
            bool (ApiObject::*)(T) const, T);
        template<typename T> std::set<const ApiFunc*> filterFuncs(
            bool (ApiFunc::*)(T) const, T);
        template<typename T> std::set<const ApiFunc*> filterFuncs(
            std::set<const ApiFunc*>, bool (ApiFunc::*)(T) const, T);
        const ApiFunc* getFuncByName(std::string);

        void addInstr(std::string);
        void addObj(const ApiObject*);
        void addType(const ApiType*);
        void addFunc(const ApiFunc*);

        virtual const ApiObject* generateSet() = 0;

    protected:
        ApiObject* generateApiObjectAndDecl(std::string, std::string,
            std::string, std::initializer_list<std::string>);
        const ApiObject* generateApiObject(std::string, const ApiType*,
            std::vector<const ApiObject*>);
        const ApiObject* generateApiObject(std::string, const ApiType*,
            const ApiFunc*, std::vector<const ApiObject*>);
        void applyFunc(const ApiFunc*, const ApiObject*, const ApiObject*,
            std::vector<const ApiObject*>);
        void applyFunc(const ApiFunc*, const ApiObject*, const ApiObject*);
        std::vector<const ApiObject*> getFuncArgs(const ApiFunc*);
};

class ApiFuzzerNew : public ApiFuzzer {
    public:
        ApiFuzzerNew(std::string config_file_path);
        //~ApiFuzzerNew();

    private:
        std::vector<unsigned int> inputs;

        void initTypes(YAML::Node, YAML::Node);
        void initFuncs(YAML::Node);
        void runGeneration(YAML::Node);
        void generateForLoop(YAML::Node);
        void generateConstructor(YAML::Node);
        void generateFunc(YAML::Node);
};

class ApiFuzzerISL : public ApiFuzzer {
    public:
        const unsigned int dims;
        const unsigned int params;
        const unsigned int constraints;

        ApiFuzzerISL(const unsigned int, const unsigned int, const unsigned int);
        ~ApiFuzzerISL();

        virtual const ApiObject* generateSet();

    private:

        std::vector<const ApiObject*> dim_var_list;

        std::vector<const ApiObject*> getDimVarList();
        void addDimVar(const ApiObject*);

        void initFuncs();
        void initTypes();
        void clearObjs();
        void clearFuncs();
        void clearTypes();

        virtual const ApiObject* generateObject(const ApiType*);
        const ApiObject* generateObject(std::string, std::string);
        const ApiObject* getCtx();
        const ApiObject* generateContext();
        const ApiObject* generateDimVar(const ApiObject*, std::string, const unsigned int);
        const ApiObject* getRandomDimVar();
        const ApiObject* getExistingVal();
        const ApiObject* generateVal();
        const ApiObject* generateSimpleVal();
        void augmentVal(const ApiObject*);
        void augmentConstraint(const ApiObject*);
        void addConstraintFromSet(const ApiObject*, const ApiObject*);
        const ApiObject* generatePWAff(const ApiObject*);
        void applyPWAFunc(const ApiObject*, std::string, std::initializer_list<std::string>);
        const ApiObject* generateSetFromConstraints(const ApiObject*, const ApiObject*);
};

#endif

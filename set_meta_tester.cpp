#include "set_meta_tester.hpp"

namespace set_meta_tester {

int indent = 0;

void
write_line(std::stringstream &ss, std::string line)
{
    int indent_count = 0;
    while (indent_count++ < indent)
        ss << "\t";
    ss << line << std::endl;
}

void
write_args(std::stringstream &ss, isl_tester::Arguments args)
{
    write_line(ss, "// Seed: " + std::to_string(args.seed));
    write_line(ss, "// Max dims: " + std::to_string(args.max_dims));
    write_line(ss, "// Max params: " + std::to_string(args.max_params));
    write_line(ss, "// Max set count: " + std::to_string(args.max_set_count));
    write_line(ss, "");
}

void
prepare_header(std::stringstream &ss)
{
    std::vector<std::string> include_list = {
        "\"isl-noexceptions.h\"",
        "<cassert>",
    };
    for (std::string incl : include_list)
        write_line(ss, "#include " + incl);
}

void
main_pre_setup(std::stringstream &ss)
{
    write_line(ss, "int main()");
    write_line(ss, "{");
    indent++;
    write_line(ss, "isl_ctx *ctx_ptr = isl_ctx_alloc();");
    write_line(ss, "{");
    indent++;
    write_line(ss, "isl::ctx ctx(ctx_ptr);");
}

void
gen_var_declarations(std::stringstream &ss, isl::set set)
{
    write_line(ss, "isl::set s = isl::set(ctx, \"" + set.to_str() + "\");");
    write_line(ss, "isl::set u = isl::set::universe(s.get_space());");
    write_line(ss, "isl::set e = isl::set::empty(s.get_space());");
}


void
main_post_setup(std::stringstream &ss)
{
    write_line(ss, "assert(r1.is_equal(r2));");
    indent--;
    write_line(ss, "}");
    write_line(ss, "isl_ctx_free(ctx_ptr);");
    indent--;
    write_line(ss, "}");
}

std::string
get_generator(const YAML::Node generator_list, std::string type)
{
    YAML::Node typed_generator_list;
    try {
        typed_generator_list = generator_list[type];
    } catch (YAML::KeyNotFound) {
        std::cout << "Could not find given type " << type << std::endl;
        exit(1);
    }
    return typed_generator_list[std::rand() % typed_generator_list.size()]
            .as<std::string>();
}

std::queue<std::string>
gen_meta_relation(const YAML::Node relation_list, unsigned int count)
{
    std::queue<std::string> meta_relation;
    while (count-- > 0) {
        int relation_id = std::rand() % relation_list.size();
        YAML::const_iterator it = relation_list.begin();
        while (relation_id-- > 0)
            it++;
        meta_relation.push(it->first.as<std::string>());
    }
    return meta_relation;
}

std::string
get_relation(const YAML::Node relation_list, std::string relation_type)
{
    const YAML::Node selected_relation_list = relation_list[relation_type];
    return selected_relation_list[std::rand() % selected_relation_list.size()]
            .as<std::string>();
}

std::string
gen_meta_func(const YAML::Node meta_list, std::queue<std::string> meta_relation)
{
    std::string rel = "%I";
    while (!meta_relation.empty()) {
        std::string new_rel = get_relation(meta_list["relations"],
                                            meta_relation.front());
        assert(new_rel.find("%1") != std::string::npos);
        int pos = 0;
        while (pos < new_rel.size()) {
            pos = new_rel.find("%", pos);
            if (pos == std::string::npos)
                break;
            else if (std::isdigit(new_rel[pos + 1]))
                new_rel = new_rel.replace(pos, 2, rel);
            pos++;
        }
        rel = new_rel;
        meta_relation.pop();
    }
    std::cout << rel << std::endl;
    while (true) {
        int pos = rel.find("%");
        if (pos == std::string::npos)
            break;
        char type = rel[pos + 1];
        if (std::isdigit(type))
            rel.replace(pos, 2, "s");
        else
            switch (type) {
                case 'I': rel.replace(pos, 2, "s"); break;
                case 'e': rel.replace(pos, 2,
                    get_generator(meta_list["generators"], "empty")); break;
                case 'u': rel.replace(pos, 2,
                    get_generator(meta_list["generators"], "universe")); break;
            }
    }
    if (rel[0] == '.')
        rel.insert(0, "s");
    std::cout << rel << std::endl;
    return rel;
}

std::pair<std::string, std::string>
gen_pair_exprs(const YAML::Node meta_list, std::queue<std::string> meta_rel)
{
    std::string expr1 = gen_meta_func(meta_list, meta_rel);
    std::string expr2;
    do {
        expr2 = gen_meta_func(meta_list, meta_rel);
    } while (!expr1.compare(expr2));
    return std::pair<std::string, std::string>(expr1, expr2);
}

void
run_simple(isl::set set_in, isl_tester::Arguments &args)
{
    YAML::Node meta_list = YAML::LoadFile("set_meta_tests.yaml");
    std::string variant = "single_distinct";

    std::stringstream ss;
    write_args(ss, args);
    prepare_header(ss);
    ss << std::endl;
    main_pre_setup(ss);
    gen_var_declarations(ss, set_in);

    unsigned int meta_rel_count = std::rand() % 5 + 1;
    std::string r1_expr, r2_expr;
    std::queue<std::string> meta_rel = gen_meta_relation(
                                        meta_list["relations"], meta_rel_count);
    if (!variant.compare("single_random")) {
        r1_expr = gen_meta_func(meta_list, meta_rel);
        r2_expr = gen_meta_func(meta_list, meta_rel);
    }
    else if (!variant.compare("single_distinct")) {
        std::pair<std::string, std::string> exprs =
            gen_pair_exprs(meta_list, meta_rel);
        r1_expr = exprs.first;
        r2_expr = exprs.second;
    }
    write_line(ss, "isl::set r1 = " + r1_expr +  ";");
    write_line(ss, "isl::set r2 = " + r2_expr +  ";");

    main_post_setup(ss);

    std::ofstream ofs;
    ofs.open("out/test.cpp");
    ofs << ss.rdbuf();
    ofs.close();
}

}

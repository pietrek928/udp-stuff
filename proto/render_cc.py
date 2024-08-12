from enum import Enum
from inspect import getdoc
from .descr import (
    ArrayField, BoolField, EnumField, FieldDescr,
    FloatField, IntField, StringField, StructDescr,
    StructField, UintField, UnionField, get_enum_int_mapping
)


# TODO: sort structs by dependencies
# TODO: sort fields by size ?

#
# Render struct definitions
#

def get_type_name(field: FieldDescr):
    if isinstance(field, EnumField):
        return field.enum.__name__
    elif isinstance(field, UintField):
        if field.bits <= 8:
            return "uint8_t"
        elif field.bits <= 16:
            return "uint16_t"
        elif field.bits <= 32:
            return "uint32_t"
        elif field.bits <= 64:
            return "uint64_t"
        else:
            raise ValueError(f"Unsupported uint field size: {field.bits}")
    elif isinstance(field, IntField):
        if field.bits <= 8:
            return "int8_t"
        elif field.bits <= 16:
            return "int16_t"
        elif field.bits <= 32:
            return "int32_t"
        elif field.bits <= 64:
            return "int64_t"
        else:
            raise ValueError(f"Unsupported int field size: {field.bits}")
    elif isinstance(field, FloatField):
        if field.bits <= 32:
            return "float"
        elif field.bits <= 64:
            return "double"
        else:
            raise ValueError(f"Unsupported float field size: {field.bits}")
    elif isinstance(field, BoolField):
        return "bool"
    elif isinstance(field, StringField):
        return "std::string"
    elif isinstance(field, EnumField):
        return field.enum.__name__
    elif isinstance(field, ArrayField):
        return f"std::vector<{get_type_name(field.item)}>"
    elif isinstance(field, StructField):
        return f"{field.struct.name}"
    elif isinstance(field, UnionField):
        return "union {\n" + '\n'.join(
            f'    {get_type_name(struct)} {struct.name}_variant;'
            for struct in field.structs
        ) + "\n}"
    else:
        raise ValueError(f"Unsupported field type: {field}")


def render_struct(struct: StructDescr):
    if struct.description:
        yield f'/* {struct.description} */'
    yield f"typedef struct {struct.name} {{"
    for field in struct.fields:
        if field.description:
            yield f"    /* {field.description} */"
        yield f"    {get_type_name(field)} {field.name};"
    yield f"}} {struct.name};"


def render_enum(enum: Enum):
    description = getdoc(enum)
    if description:
        yield f'/* {description} */'
    yield "typedef enum {"
    for name, value in get_enum_int_mapping(enum):
        description = getdoc(enum.__members__[name])
        yield f"    {name} = {value}," + (
            f" /* {description} */" if description else ""
        )


#
# Read functions
#

def render_parse_array(array: ArrayField, reader, result):
    size_type = get_type_name(array.size)
    yield "{"
    yield f"    {size_type} size;"
    yield f"    {reader}.read_uint({array.size.bits}, &size);"
    yield f"    {result}.resize(size);"
    yield f"    for (auto &vec_item : {result}) {{"
    yield from render_parse_field(array.item, reader, "vec_item")
    yield "    }"
    yield "}"


def render_parse_struct_field(struct: StructField, reader, result):
    if struct.length is not None:
        length_type = get_type_name(struct.length)
        yield "{"
        yield f"    {length_type} length;"
        yield f"    {reader}.read_uint({struct.length.bits}, &length);"
        yield f"    auto struct_reader = reader.read_fragment(length);"
        yield f"    try {{"
        yield f"        parse_{struct.struct.name}(struct_reader, {result}.{struct.name});"
        yield f"    }} catch (BitStreamFinished &e) {{"
        yield f"    }}"
        yield "}"
    else:
        yield f"parse_{struct.struct.name}(reader, {result}.{struct.name});"


def render_parse_union_field(union: UnionField, reader, result):
    type_type = get_type_name(union.type_field)
    yield "{"
    yield f"    {type_type} type;"
    yield f"    {reader}.read_uint({union.type_field.bits}, &type);"
    if union.length is not None:
        length_type = get_type_name(union.length)
        yield f"    {length_type} length;"
        yield f"    {reader}.read_uint({union.length.bits}, &length);"
        yield f"    auto struct_reader = reader.read_fragment(length);"
        yield f"    try {{"
        yield f"        switch (type) {{"
        for n, struct in enumerate(union.structs):
            yield f"        case {n}:"
            yield f"            parse_{struct.name}(struct_reader, {result}.{struct.name}_variant);"
            yield f"            break;"
        yield "        default:"
        yield "            throw std::runtime_error(\"Unknown union type\");"
        yield "        }"
        yield f"    }} catch (BitStreamFinished &e) {{"
        yield f"    }}"
    else:
        yield f"    switch (type) {{"
        for n, struct in enumerate(union.structs):
            yield f"    case {n}: {{"
            yield f"       parse_{struct.name}(reader, {result}.{struct.name}_variant);"
            yield "        break;"
        yield "    default:"
        yield "        throw std::runtime_error(\"Unknown union type\");"
        yield "    }"
    yield "}"


def render_parse_field(field: FieldDescr, reader, result):
    if field.default is not None:
        yield f"if (read_field_flag({reader})) {{"

    if isinstance(field, EnumField):
        yield f"    read_enum_field(reader, &{result}, {field.bits});"
    elif isinstance(field, UintField):
        yield f"    {reader}.read_uint({field.bits}, &{result});"
    elif isinstance(field, IntField):
        yield f"    {reader}.read_int({field.bits}, &{result});"
    elif isinstance(field, FloatField):
        yield f"    {reader}.read_float({field.bits}, &{result});"
    elif isinstance(field, BoolField):
        yield f"    {reader}.read_bool(&{result});"
    elif isinstance(field, StringField):
        yield f"    read_string_field(reader, &{result}, {field.size.bits});"
    elif isinstance(field, ArrayField):
        yield from render_parse_array(field, reader, result)
    elif isinstance(field, StructField):
        yield from render_parse_struct_field(field, reader, result)
    elif isinstance(field, UnionField):
        yield from render_parse_union_field(field, reader, result)
    else:
        raise ValueError(f"Unsupported field type: {field}")

    if field.default is not None:
        yield "}"


def render_parse_struct(struct: StructDescr):
    yield f"template<class Treader>"
    yield f"void parse_{struct.name}(Treader &reader, {struct.name} &result) {{"
    for field in struct.fields:
        yield from render_parse_field(field, "reader", f"result.{field.name}")
    yield "}"


#
# Write functions
#

def render_write_array(array: ArrayField, writer, value):
    yield "{"
    yield f"    {writer}.write_uint({array.size.bits}, {value}.size());"
    yield f"    for (const auto &vec_item : {value}) {{"
    yield from render_write_field(array.item, writer, "vec_item")
    yield "    }"
    yield "}"


def render_write_struct_field(struct: StructField, writer, value):
    if struct.length is not None:
        yield "{"
        yield f"    {writer}.write_uint({struct.length.bits}, 0);"
        yield f"    auto struct_pos = {writer}.get_bit_pos();"
        yield f"    write_{struct.struct.name}(writer, {value}.{struct.name});"
        yield f"    auto length = {writer}.get_bit_pos() - struct_pos;"
        yield f"    {writer}.write_uint_at(struct_pos - {struct.length.bits}, {struct.length.bits}, length);"
        yield "}"
    else:
        yield f"write_{struct.struct.name}(writer, {value}.{struct.name});"


def render_write_union_field(union: UnionField, writer, value):
    yield "{"
    yield f"    {writer}.write_uint({union.type_field.bits}, 0);"
    if union.length is not None:
        yield f"    {writer}.write_uint({union.length.bits}, 0);"
        yield f"    auto struct_pos = {writer}.get_bit_pos();"
    yield f"    switch ({value}.{union.type_field.name}) {{"
    for n, struct in enumerate(union.structs):
        yield f"    case {n}:"
        yield f"        write_{struct.name}(writer, {value}.{struct.name}_variant);"
        yield f"        break;"
    yield "    default:"
    yield "        throw std::runtime_error(\"Unknown union type\");"
    yield "    }"
    if union.length is not None:
        yield f"    auto struct_length = {writer}.get_bit_pos() - struct_pos;"
        yield f"    {writer}.write_uint_at(struct_pos - {union.length.bits}, {union.length.bits}, struct_length);"
    yield "}"


def render_write_field(field: FieldDescr, writer, value):
    if field.default is not None:
        yield f"if ({value} != {field.default}) {{"

    if isinstance(field, EnumField):
        yield f"    write_enum_field(writer, {value}, {field.bits});"
    elif isinstance(field, UintField):
        yield f"    {writer}.write_uint({field.bits}, {value});"
    elif isinstance(field, IntField):
        yield f"    {writer}.write_int({field.bits}, {value});"
    elif isinstance(field, FloatField):
        yield f"    {writer}.write_float({field.bits}, {value});"
    elif isinstance(field, BoolField):
        yield f"    {writer}.write_bool({value});"
    elif isinstance(field, StringField):
        yield f"    write_string_field(writer, {value}, {field.size.bits});"
    elif isinstance(field, ArrayField):
        yield from render_write_array(field, writer, value)
    elif isinstance(field, StructField):
        yield from render_write_struct_field(field, writer, value)
    elif isinstance(field, UnionField):
        yield from render_write_union_field(field, writer, value)
    else:
        raise ValueError(f"Unsupported field type: {field}")

    if field.default is not None:
        yield "}"


def render_write_struct(struct: StructDescr):
    yield f"template<class Twriter>"
    yield f"void write_{struct.name}(Twriter &writer, const {struct.name} &value) {{"
    for field in struct.fields:
        yield from render_write_field(field, "writer", f"value.{field.name}")
    yield "}"

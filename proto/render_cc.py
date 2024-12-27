from enum import Enum
from inspect import getdoc
from .descr import (
    ArrayField, BoolField, EnumField, FieldDescr,
    FloatField, IdentEnd, IdentStart, Sep, IntField, StringField, StructDescr,
    StructField, UintField, UnionDescr, UnionField, get_enum_int_mapping
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
        return f"{field.union.name}"
    else:
        raise ValueError(f"Unsupported field type: {field}")


def render_struct(struct: StructDescr):
    if struct.description:
        yield f'/* {struct.description} */'
    yield f"typedef struct {struct.name} {{"
    yield IdentStart
    for field in struct.fields:
        if field.description:
            yield f"/* {field.description} */"
        if isinstance(field, UnionField):
            yield f"{field.union.name}_enum_class {field.name}_type;"
        yield f"{get_type_name(field)} {field.name};"
    yield IdentEnd
    yield f"}} {struct.name};"
    yield Sep


def render_enum(enum: Enum):
    description = getdoc(enum)
    if description:
        yield f'/* {description} */'
    yield f"enum class {enum.__name__} {{"
    yield IdentStart
    for name, value in get_enum_int_mapping(enum):
        description = getdoc(enum.__members__[name])
        yield f"{name} = {value}," + (
            f" /* {description} */" if description else ""
        )
    yield IdentEnd
    yield f"}};"
    yield Sep


def render_union(union: UnionDescr):
    yield f"typedef union {union.name} {{"
    yield IdentStart
    for struct in union.structs:
        yield f"{struct.name} {struct.name}_variant;"
    yield IdentEnd
    yield f"}} {union.name};"
    yield Sep


def render_union_enum(union: UnionDescr):
    yield f"enum class {union.name}_enum_class {{"
    yield IdentStart
    if union.allow_empty:
        yield f"{union.name}_empty = 0,"
        n = 1
    else:
        n = 0
    for struct in union.structs:
        yield f"{struct.name} = {n},"
        n += 1
    yield IdentEnd
    yield f"}};"
    yield Sep


#
# Read functions
#

def render_parse_array(array: ArrayField, reader, result):
    yield "{"
    yield IdentStart
    if array.const_size is not None:
        yield f"{result}.resize({array.const_size});"
    else:
        size_type = get_type_name(array.size)
        yield f"{size_type} size;"
        yield f"{reader}.read_uint({array.size.bits}, &size);"
        yield f"{result}.resize(size);"
    yield f"for (auto &vec_item : {result}) {{"
    yield IdentStart
    yield from render_parse_field(array.item, reader, "vec_item")
    yield IdentEnd
    yield "}"
    yield IdentEnd
    yield "}"


def render_parse_struct_field(struct: StructField, reader, result):
    if struct.length is not None:
        length_type = get_type_name(struct.length)
        yield "{"
        yield IdentStart
        yield f"{length_type} length;"
        yield f"{reader}.read_uint({struct.length.bits}, &length);"
        yield f"auto struct_reader = reader.read_fragment(length);"
        yield f"try {{"
        yield IdentStart
        yield f"parse_{struct.struct.name}(struct_reader, {result});"
        yield IdentEnd
        yield f"}} catch (BitStreamFinished &e) {{"
        yield f"}}"
        yield IdentEnd
        yield "}"
    else:
        yield f"parse_{struct.struct.name}(reader, {result});"


def render_parse_union_field(union: UnionField, reader, result, type_var):
    yield f"{reader}.read_uint({union.type_field.bits}, &{type_var});"
    if union.length is not None:
        length_type = get_type_name(union.length)
        if union.union.allow_empty:
            yield f"if ({type_var} != {union.union.name}_empty) {{"
            yield IdentStart
        yield f"{length_type} length;"
        yield f"{reader}.read_uint({union.length.bits}, &length);"
        yield f"auto struct_reader = reader.read_fragment(length);"
        yield f"try {{"
        yield IdentStart
        yield f"switch ({type_var}) {{"
        yield IdentStart
        for struct in union.union.structs:
            yield f"case {union.union.name}_enum_class::{struct.name}: {{"
            yield IdentStart
            yield f"parse_{struct.name}(struct_reader, {result}.{struct.name}_variant);"
            yield "} break;"
            yield IdentEnd
        yield "default:"
        yield IdentStart
        yield "throw std::runtime_error(\"Unknown union type\");"
        yield IdentEnd
        yield "}"
        yield IdentEnd
        yield f"}} catch (BitStreamFinished &e) {{"
        yield f"}}"
        if union.union.allow_empty:
            yield IdentEnd
            yield "}"
    else:
        yield f"switch ({type_var}) {{"
        yield IdentStart
        if union.union.allow_empty:
            yield f"case {union.union.name}_empty:"
            yield IdentStart
            yield f"break;"
            yield IdentEnd
        for struct in union.union.structs:
            yield f"case {union.union.name}_enum_class::{struct.name}: {{"
            yield IdentStart
            yield f"parse_{struct.name}(reader, {result}.{struct.name}_variant);"
            yield "} break;"
            yield IdentEnd
        yield "default:"
        yield IdentStart
        yield "throw std::runtime_error(\"Unknown union type\");"
        yield IdentEnd
        yield IdentEnd
        yield "}"


def render_parse_field(field: FieldDescr, reader, result):
    if field.default is not None:
        yield f"if (read_field_flag({reader})) {{"
        yield IdentStart

    if isinstance(field, EnumField):
        yield f"read_enum_field(reader, &{result}, {field.bits});"
    elif isinstance(field, UintField):
        yield f"{reader}.read_uint({field.bits}, &{result});"
    elif isinstance(field, IntField):
        yield f"{reader}.read_int({field.bits}, &{result});"
    elif isinstance(field, FloatField):
        yield f"{reader}.read_float({field.bits}, &{result});"
    elif isinstance(field, BoolField):
        yield f"{reader}.read_bool(&{result});"
    elif isinstance(field, StringField):
        yield f"read_string_field(reader, &{result}, {field.size.bits});"
    elif isinstance(field, ArrayField):
        yield from render_parse_array(field, reader, result)
    elif isinstance(field, StructField):
        yield from render_parse_struct_field(field, reader, result)
    elif isinstance(field, UnionField):
        yield from render_parse_union_field(field, reader, result, f"{result}_type")
    else:
        raise ValueError(f"Unsupported field type: {field}")

    if field.default is not None:
        yield IdentEnd
        yield "}"


def render_parse_struct(struct: StructDescr):
    yield f"template<class Treader>"
    yield f"void parse_{struct.name}(Treader &reader, {struct.name} &result) {{"
    yield IdentStart
    for field in struct.fields:
        yield from render_parse_field(field, "reader", f"result.{field.name}")
    yield IdentEnd
    yield "}"
    yield Sep


#
# Write functions
#

def render_write_array(array: ArrayField, writer, value):
    yield "{"
    yield IdentStart
    if array.const_size is None:
        yield f"{writer}.write_uint({array.size.bits}, {value}.size());"
    else:
        yield (
            f"if ({value}.size() != {array.const_size}) "
            f"throw std::invalid_argument(\"Array size must be {array.const_size}\");"
        )
    yield f"for (const auto &vec_item : {value}) {{"
    yield IdentStart
    yield from render_write_field(array.item, writer, "vec_item")
    yield IdentEnd
    yield "}"
    yield IdentEnd
    yield "}"


def render_write_struct_field(struct: StructField, writer, value):
    if struct.length is not None:
        yield "{"
        yield IdentStart
        yield f"{writer}.write_uint({struct.length.bits}, 0);"
        yield f"auto struct_pos = {writer}.get_bit_pos();"
        yield f"write_{struct.struct.name}(writer, {value});"
        yield f"auto length = {writer}.get_bit_pos() - struct_pos;"
        yield f"{writer}.write_uint_at(struct_pos - {struct.length.bits}, {struct.length.bits}, length);"
        yield IdentEnd
        yield "}"
    else:
        yield f"write_{struct.struct.name}(writer, {value});"


def render_write_union_field(union: UnionField, writer, value, type_var):
    yield "{"
    yield IdentStart
    yield f"{writer}.write_uint({union.type_field.bits}, {type_var});"
    if union.union.allow_empty:
        yield f"if ({type_var} != {union.union.name}_empty) {{"
        yield IdentStart
    if union.length is not None:
        yield f"{writer}.write_uint({union.length.bits}, 0);"
        yield f"auto struct_pos = {writer}.get_bit_pos();"
    yield f"switch ({type_var}) {{"
    yield IdentStart
    for n, struct in enumerate(union.union.structs):
        yield f"case {union.union.name}_enum_class::{struct.name}:"
        yield IdentStart
        yield f"write_{struct.name}(writer, {value}.{struct.name}_variant);"
        yield f"break;"
        yield IdentEnd
    yield "default:"
    yield IdentStart
    yield "throw std::runtime_error(\"Unknown union type\");"
    yield IdentEnd
    yield IdentEnd
    yield "}"
    if union.length is not None:
        yield f"auto struct_length = {writer}.get_bit_pos() - struct_pos;"
        yield f"{writer}.write_uint_at(struct_pos - {union.length.bits}, {union.length.bits}, struct_length);"
    if union.union.allow_empty:
        yield IdentEnd
        yield "}"
    yield IdentEnd
    yield "}"


def render_write_field(field: FieldDescr, writer, value):
    if field.default is not None:
        yield f"if ({field.check_default(value)}) {{"
        yield IdentStart

    if isinstance(field, EnumField):
        yield f"write_enum_field(writer, {value}, {field.bits});"
    elif isinstance(field, UintField):
        yield f"{writer}.write_uint({field.bits}, {value});"
    elif isinstance(field, IntField):
        yield f"{writer}.write_int({field.bits}, {value});"
    elif isinstance(field, FloatField):
        yield f"{writer}.write_float({field.bits}, {value});"
    elif isinstance(field, BoolField):
        yield f"{writer}.write_bool({value});"
    elif isinstance(field, StringField):
        yield f"write_string_field(writer, {value}, {field.size.bits});"
    elif isinstance(field, ArrayField):
        yield from render_write_array(field, writer, value)
    elif isinstance(field, StructField):
        yield from render_write_struct_field(field, writer, value)
    elif isinstance(field, UnionField):
        yield from render_write_union_field(field, writer, value, f"{value}_type")
    else:
        raise ValueError(f"Unsupported field type: {field}")

    if field.default is not None:
        yield IdentEnd
        yield "}"


def render_write_struct(struct: StructDescr):
    yield f"template<class Twriter>"
    yield f"void write_{struct.name}(Twriter &writer, const {struct.name} &value) {{"
    yield IdentStart
    for field in struct.fields:
        yield from render_write_field(field, "writer", f"value.{field.name}")
    yield IdentEnd
    yield "}"
    yield Sep


def render_header_begin():
    yield "#pragma once"
    yield Sep
    yield "#include <cstdint>"
    yield "#include <string>"
    yield "#include <vector>"
    yield "#include <stdexcept>"
    yield Sep

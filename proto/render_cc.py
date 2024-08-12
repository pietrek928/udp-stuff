from enum import Enum
from inspect import getdoc
from .descr import ArrayField, BoolField, EnumField, FieldDescr, FloatField, IntField, StringField, StructDescr, StructField, UintField, get_enum_int_mapping


# TODO: sort structs by dependencies
# TODO: sort fields by size ?


def get_type_name(field: FieldDescr):
    if isinstance(field, UintField):
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
    elif isinstance(field, EnumField):
        return field.enum.__name__
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


def render_parse_struct(struct: StructField, reader, result):
    if struct.length:
        length_type = get_type_name(struct.length)
        yield "{"
        yield f"    {length_type} length;"
        yield f"    {reader}.read_uint({struct.length.bits}, &length);"
        yield f"    parse_{struct.struct.name}(reader.read_fragment(length), {result}.{struct.name});"
        yield "}"
    else:
        yield f"parse_{struct.struct.name}(reader, &{result}.{struct.name});"


def render_parse_field(field: FieldDescr, reader, result):
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
        yield from render_parse_struct(field, reader, result)


def render_parse_struct(struct: StructDescr):
    yield f"template<class Treader>"
    yield f"void parse_{struct.name}(Treader &reader, {struct.name} &result) {{"
    for field in struct.fields:
        yield from render_parse_field(field, "reader", f"result.{field.name}")
    yield "}"

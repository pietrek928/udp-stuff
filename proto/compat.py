from enum import Enum
from typing import Tuple
from .descr import ArrayField, BoolField, EnumField, FieldDescr, FloatField, IntField, StringField, StructDescr, StructField, UintField, UnionDescr, UnionField, get_enum_int_mapping


BITS_FIELDS = {
    BoolField, UintField, IntField, FloatField, EnumField
}


def validate_field(f1: FieldDescr, f2: FieldDescr, path: Tuple[str, ...]):
    if isinstance(f1, BITS_FIELDS) and isinstance(f2, BITS_FIELDS):
        if f1.bits != f2.bits:
            raise ValueError(f"Field bit size mismatch: {path} {f1.bits} != {f2.bits}")
        if type(f1) is not type(f2):
            if isinstance(f1, FloatField) or isinstance(f2, FloatField):
                raise ValueError(f"Incompatible field types: {path} {type(f1)} {type(f2)}")
            if isinstance(f1, EnumField) and isinstance(f2, EnumField):
                pass  # TODO: compare enum fields
    if f1.name != f2.name:
        # TODO: exception here ?
        print('WARING: Field name mismatch:', path, f1.name, f2.name)
        # raise ValueError(f"Field name mismatch: {path} {f1.name} != {f2.name}")
    if type(f1) is not type(f2):
        raise ValueError(f"Field type mismatch: {path} {type(f1)} != {type(f2)}")
    if isinstance(f1, StringField):
        validate_field(f1.size, f2.size, path + ('size',))
    elif isinstance(f1, ArrayField):
        validate_field(f1.item, f2.item, path + ('item',))
        validate_field(f1.size, f2.size, path + ('size',))
    elif isinstance(f1, UnionField):
        validate_field(f1.type_field, f2.type_field, path + ('type_field',))
        if f1.length is not None and f2.length is not None:
            validate_field(f1.length, f2.length, path + ('length',))
        elif f1.length is not None or f2.length is not None:
            raise ValueError(f"Field length mismatch: {path} {f1.length} != {f2.length}")
        validate_union(
            f1.union, f2.union, path + (f1.union.name,),
            match_length=f1.length is None
        )
    elif isinstance(f1, StructField):
        if f1.length is not None and f2.length is not None:
            validate_field(f1.length, f2.length, path + ('length',))
        elif f1.length is not None or f2.length is not None:
            raise ValueError(f"Field length mismatch: {path} {f1.length} != {f2.length}")
        validate_struct(
            f1.struct, f2.struct, path + (f1.struct.name,),
            match_length=f1.length is None
        )


def validate_enum(e1: Enum, e2: Enum, path: Tuple[str, ...]):
    if len(e1.__members__) != len(e2.__members__):
        raise ValueError(f"Enum member count mismatch: {path} {len(e1.__members__)} != {len(e2.__members__)}")
    e1_mapping = dict(get_enum_int_mapping(e1))
    e2_mapping = dict(get_enum_int_mapping(e2))
    for k in set(e1_mapping) & set(e2_mapping):
        if e1_mapping[k] != e2_mapping[k]:
            raise ValueError(f"Enum values mismatch: {path} {k} {e1_mapping[k]} != {e2_mapping[k]}")
    e1_mapping_rev = {
        v: k for k, v in e1_mapping.items()
    }
    e2_mapping_rev = {
        v: k for k, v in e2_mapping.items()
    }
    for k in set(e1_mapping_rev) & set(e2_mapping_rev):
        if e1_mapping_rev[k] != e2_mapping_rev[k]:
            raise ValueError(f"Enum values numbers mismatch: {path} {k} {e1_mapping_rev[k]} != {e2_mapping_rev[k]}")


def validate_union(
    u1: UnionDescr, u2: UnionDescr, path: Tuple[str, ...],
    match_length=False
):
    if u1.allow_empty != u2.allow_empty:
        raise ValueError(f"Union allow_empty mismatch: {path} {u1.allow_empty} != {u2.allow_empty}")
    if match_length:
        if len(u1.structs) != len(u2.structs):
            raise ValueError(f"Union struct count mismatch: {path} {len(u1.structs)} != {len(u2.structs)}")
    else:
        if len(u1.structs) > len(u2.structs):
            raise ValueError(f"You cannot remove from union in same proto version: {path} {len(u1.structs)} > {len(u2.structs)}")
    for s1, s2 in zip(u1.structs, u2.structs):
        validate_struct(s1, s2, path + (s1.name,), match_length=match_length)


def validate_struct(
    s1: StructDescr, s2: StructDescr, path: Tuple[str, ...],
    match_length=False
):
    n = min(len(s1.fields), len(s2.fields))
    for i in range(n):
        validate_field(s1.fields[i], s2.fields[i], path + (s1.fields[i].name,))
    if match_length:
        if len(s1.fields) != len(s2.fields):
            raise ValueError(f"Struct size mismatch: {path} {len(s1.fields)} != {s2.fields}")

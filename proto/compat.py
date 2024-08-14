from typing import Tuple
from .descr import BoolField, EnumField, FieldDescr, FloatField, IntField, StructDescr, UintField


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
    # if f1.name != f2.name:  TODO: allow name change ????
    #     raise ValueError(f"Field name mismatch: {path} {f1.name} != {f2.name}")
    


def validate_struct(s1: StructDescr, s2: StructDescr, path: Tuple[str, ...]):
    n = min(len(s1.fields), len(s2.fields))
    for i in range(n):
        validate_field(s1.fields[i], s2.fields[i], path + (s1.fields[i].name,))

from proto.descr import StructDescr, UintField, UnionDescr, UnionField
from proto.render_all import render_proto

register = StructDescr(
    name='register',
    fields=[
        UintField(name='ipv4', bits=32, default=0),
    ]
)

tracker = UnionDescr(
    name='tracker',
    structs=[
    ]
)

tracker_frame = StructDescr(
    name='tracker',
    fields=[
        UnionField(
            name='tracker',
            union=
        )
    ]
)

render_proto((tracker_frame, ), 'proto')

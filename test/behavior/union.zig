const std = @import("std");
const expect = std.testing.expect;
const expectEqual = std.testing.expectEqual;
const Tag = std.meta.Tag;

const Foo = union {
    float: f64,
    int: i32,
};

test "basic unions" {
    var foo = Foo{ .int = 1 };
    try expect(foo.int == 1);
    foo = Foo{ .float = 12.34 };
    try expect(foo.float == 12.34);
}

test "init union with runtime value" {
    var foo: Foo = undefined;

    setFloat(&foo, 12.34);
    try expect(foo.float == 12.34);

    setInt(&foo, 42);
    try expect(foo.int == 42);
}

fn setFloat(foo: *Foo, x: f64) void {
    foo.* = Foo{ .float = x };
}

fn setInt(foo: *Foo, x: i32) void {
    foo.* = Foo{ .int = x };
}

test "comptime union field access" {
    comptime {
        var foo = Foo{ .int = 0 };
        try expect(foo.int == 0);

        foo = Foo{ .float = 42.42 };
        try expect(foo.float == 42.42);
    }
}

const FooExtern = extern union {
    float: f64,
    int: i32,
};

test "basic extern unions" {
    var foo = FooExtern{ .int = 1 };
    try expect(foo.int == 1);
    foo.float = 12.34;
    try expect(foo.float == 12.34);
}

const ExternPtrOrInt = extern union {
    ptr: *u8,
    int: u64,
};
test "extern union size" {
    comptime try expect(@sizeOf(ExternPtrOrInt) == 8);
}

test "0-sized extern union definition" {
    const U = extern union {
        a: void,
        const f = 1;
    };

    try expect(U.f == 1);
}

const Value = union(enum) {
    Int: u64,
    Array: [9]u8,
};

const Agg = struct {
    val1: Value,
    val2: Value,
};

const v1 = Value{ .Int = 1234 };
const v2 = Value{ .Array = [_]u8{3} ** 9 };

const err = @as(anyerror!Agg, Agg{
    .val1 = v1,
    .val2 = v2,
});

const array = [_]Value{ v1, v2, v1, v2 };

test "unions embedded in aggregate types" {
    switch (array[1]) {
        Value.Array => |arr| try expect(arr[4] == 3),
        else => unreachable,
    }
    switch ((err catch unreachable).val1) {
        Value.Int => |x| try expect(x == 1234),
        else => unreachable,
    }
}

test "access a member of tagged union with conflicting enum tag name" {
    const Bar = union(enum) {
        A: A,
        B: B,

        const A = u8;
        const B = void;
    };

    comptime try expect(Bar.A == u8);
}

test "constant tagged union with payload" {
    var empty = TaggedUnionWithPayload{ .Empty = {} };
    var full = TaggedUnionWithPayload{ .Full = 13 };
    shouldBeEmpty(empty);
    shouldBeNotEmpty(full);
}

fn shouldBeEmpty(x: TaggedUnionWithPayload) void {
    switch (x) {
        TaggedUnionWithPayload.Empty => {},
        else => unreachable,
    }
}

fn shouldBeNotEmpty(x: TaggedUnionWithPayload) void {
    switch (x) {
        TaggedUnionWithPayload.Empty => unreachable,
        else => {},
    }
}

const TaggedUnionWithPayload = union(enum) {
    Empty: void,
    Full: i32,
};

test "union alignment" {
    comptime {
        try expect(@alignOf(AlignTestTaggedUnion) >= @alignOf([9]u8));
        try expect(@alignOf(AlignTestTaggedUnion) >= @alignOf(u64));
    }
}

const AlignTestTaggedUnion = union(enum) {
    A: [9]u8,
    B: u64,
};

const Letter = enum { A, B, C };
const Payload = union(Letter) {
    A: i32,
    B: f64,
    C: bool,
};

test "union with specified enum tag" {
    try doTest();
    comptime try doTest();
}

fn doTest() error{TestUnexpectedResult}!void {
    try expect((try bar(Payload{ .A = 1234 })) == -10);
}

fn bar(value: Payload) error{TestUnexpectedResult}!i32 {
    try expect(@as(Letter, value) == Letter.A);
    return switch (value) {
        Payload.A => |x| return x - 1244,
        Payload.B => |x| if (x == 12.34) @as(i32, 20) else 21,
        Payload.C => |x| if (x) @as(i32, 30) else 31,
    };
}

fn testComparison() !void {
    var x = Payload{ .A = 42 };
    try expect(x == .A);
    try expect(x != .B);
    try expect(x != .C);
    try expect((x == .B) == false);
    try expect((x == .C) == false);
    try expect((x != .A) == false);
}

test "comparison between union and enum literal" {
    try testComparison();
    comptime try testComparison();
}

const TheTag = enum { A, B, C };
const TheUnion = union(TheTag) {
    A: i32,
    B: i32,
    C: i32,
};
test "cast union to tag type of union" {
    try testCastUnionToTag();
    comptime try testCastUnionToTag();
}

fn testCastUnionToTag() !void {
    var u = TheUnion{ .B = 1234 };
    try expect(@as(TheTag, u) == TheTag.B);
}

test "cast tag type of union to union" {
    var x: Value2 = Letter2.B;
    try expect(@as(Letter2, x) == Letter2.B);
}
const Letter2 = enum { A, B, C };
const Value2 = union(Letter2) {
    A: i32,
    B,
    C,
};

test "implicit cast union to its tag type" {
    var x: Value2 = Letter2.B;
    try expect(x == Letter2.B);
    try giveMeLetterB(x);
}
fn giveMeLetterB(x: Letter2) !void {
    try expect(x == Value2.B);
}

// TODO it looks like this test intended to test packed unions, but this is not a packed
// union. go through git history and find out what happened.
pub const PackThis = union(enum) {
    Invalid: bool,
    StringLiteral: u2,
};

test "constant packed union" {
    try testConstPackedUnion(&[_]PackThis{PackThis{ .StringLiteral = 1 }});
}

fn testConstPackedUnion(expected_tokens: []const PackThis) !void {
    try expect(expected_tokens[0].StringLiteral == 1);
}

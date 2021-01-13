const std = @import("std");
const TestContext = @import("../../src/test.zig").TestContext;

const macos_aarch64 = std.zig.CrossTarget{
    .cpu_arch = .aarch64,
    .os_tag = .macos,
};

const linux_aarch64 = std.zig.CrossTarget{
    .cpu_arch = .aarch64,
    .os_tag = .linux,
};

pub fn addCases(ctx: *TestContext) !void {
    {
        var case = ctx.exe("hello world with updates", macos_aarch64);

        // Regular old hello world
        case.addCompareOutput(
            \\export fn _start() noreturn {
            \\    print();
            \\
            \\    exit();
            \\}
            \\
            \\fn print() void {
            \\    asm volatile ("svc #0x80"
            \\        :
            \\        : [number] "{x16}" (4),
            \\          [arg1] "{x0}" (1),
            \\          [arg2] "{x1}" (@ptrToInt("Hello, World!\n")),
            \\          [arg3] "{x2}" (14)
            \\        : "memory"
            \\    );
            \\    return;
            \\}
            \\
            \\fn exit() noreturn {
            \\    asm volatile ("svc #0x80"
            \\        :
            \\        : [number] "{x16}" (1),
            \\          [arg1] "{x0}" (0)
            \\        : "memory"
            \\    );
            \\    unreachable;
            \\}
        ,
            "Hello, World!\n",
        );
        // Now change the message only
        case.addCompareOutput(
            \\export fn _start() noreturn {
            \\    print();
            \\
            \\    exit();
            \\}
            \\
            \\fn print() void {
            \\    asm volatile ("svc #0x80"
            \\        :
            \\        : [number] "{x16}" (4),
            \\          [arg1] "{x0}" (1),
            \\          [arg2] "{x1}" (@ptrToInt("What is up? This is a longer message that will force the data to be relocated in virtual address space.\n")),
            \\          [arg3] "{x2}" (104)
            \\        : "memory"
            \\    );
            \\    return;
            \\}
            \\
            \\fn exit() noreturn {
            \\    asm volatile ("svc #0x80"
            \\        :
            \\        : [number] "{x16}" (1),
            \\          [arg1] "{x0}" (0)
            \\        : "memory"
            \\    );
            \\    unreachable;
            \\}
        ,
            "What is up? This is a longer message that will force the data to be relocated in virtual address space.\n",
        );
        // Now we print it twice.
        case.addCompareOutput(
            \\export fn _start() noreturn {
            \\    print();
            \\    print();
            \\
            \\    exit();
            \\}
            \\
            \\fn print() void {
            \\    asm volatile ("svc #0x80"
            \\        :
            \\        : [number] "{x16}" (4),
            \\          [arg1] "{x0}" (1),
            \\          [arg2] "{x1}" (@ptrToInt("What is up? This is a longer message that will force the data to be relocated in virtual address space.\n")),
            \\          [arg3] "{x2}" (104)
            \\        : "memory"
            \\    );
            \\    return;
            \\}
            \\
            \\fn exit() noreturn {
            \\    asm volatile ("svc #0x80"
            \\        :
            \\        : [number] "{x16}" (1),
            \\          [arg1] "{x0}" (0)
            \\        : "memory"
            \\    );
            \\    unreachable;
            \\}
        ,
            \\What is up? This is a longer message that will force the data to be relocated in virtual address space.
            \\What is up? This is a longer message that will force the data to be relocated in virtual address space.
            \\
        );
    }

    {
        var case = ctx.exe("hello world", linux_aarch64);
        // Regular old hello world
        case.addCompareOutput(
            \\export fn _start() noreturn {
            \\    print();
            \\    exit();
            \\}
            \\
            \\fn doNothing() void {}
            \\
            \\fn answer() u64 {
            \\    return 0x1234abcd1234abcd;
            \\}
            \\
            \\fn print() void {
            \\    asm volatile ("svc #0"
            \\        :
            \\        : [number] "{x8}" (64),
            \\          [arg1] "{x0}" (1),
            \\          [arg2] "{x1}" (@ptrToInt("Hello, World!\n")),
            \\          [arg3] "{x2}" ("Hello, World!\n".len)
            \\        : "memory", "cc"
            \\    );
            \\}
            \\
            \\fn exit() noreturn {
            \\    asm volatile ("svc #0"
            \\        :
            \\        : [number] "{x8}" (93),
            \\          [arg1] "{x0}" (0)
            \\        : "memory", "cc"
            \\    );
            \\    unreachable;
            \\}
        ,
            "Hello, World!\n",
        );
    }

    {
        var case = ctx.exe("exit fn taking argument", macos_aarch64);

        case.addCompareOutput(
            \\export fn _start() noreturn {
            \\    exit(0);
            \\}
            \\
            \\fn exit(ret: usize) noreturn {
            \\    asm volatile ("svc #0x80"
            \\        :
            \\        : [number] "{x16}" (1),
            \\          [arg1] "{x0}" (ret)
            \\        : "memory"
            \\    );
            \\    unreachable;
            \\}
        ,
            "",
        );
    }

    {
        var case = ctx.exe("exit fn taking argument", linux_aarch64);

        case.addCompareOutput(
            \\export fn _start() noreturn {
            \\    exit(0);
            \\}
            \\
            \\fn exit(ret: usize) noreturn {
            \\    asm volatile ("svc #0"
            \\        :
            \\        : [number] "{x8}" (93),
            \\          [arg1] "{x0}" (ret)
            \\        : "memory", "cc"
            \\    );
            \\    unreachable;
            \\}
        ,
            "",
        );
    }

    {
        var case = ctx.exe("hello world linked to libc", macos_aarch64);

        // TODO rewrite this test once we handle more int conversions and return args.
        case.addCompareOutput(
            \\extern "c" fn write(usize, usize, usize) void;
            \\extern "c" fn exit(usize) noreturn;
            \\
            \\export fn _start() noreturn {
            \\    write(1, @ptrToInt("Hello,"), 6);
            \\    write(1, @ptrToInt(" World!\n,"), 8);
            \\    exit(0);
            \\}
        ,
            "Hello, World!\n",
        );
    }
}

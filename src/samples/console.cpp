#include <jolt/debug/console.hpp>
#include <jolt/threading/thread.hpp>
#include <jolt/io/stream.hpp>
#include <jolt/text/string.hpp>

using namespace jolt;
using namespace jolt::debug;
using namespace jolt::io;
using namespace jolt::text;

int main(int argc, char **argv) {
    jolt::threading::initialize();

    StandardErrorStream s_err;
    Console c{nullptr, &s_err};

    c.echo(u8"This is echoed.");
    c.warn(u8"This is warned.");
    c.err(u8"This is errored.");

    c.echo(EmptyString);

    c.echo("1", false);
    c.echo(" 2", false);
    c.echo(" 3!");

    return 0;
}

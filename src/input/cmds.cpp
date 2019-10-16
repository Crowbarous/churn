#include "cmds.h"
#include "core/core.h"
#include "bind.h"
#include "render/render.h"
#include "input.h"

namespace input
{
namespace cmd
{

COMMAND_ROUTINE (nop)
{
	return;
}

COMMAND_ROUTINE (exit)
{
	core::game.exit_code = 0;
	if (!args.empty())
		core::game.exit_code = std::atoi(args[0].c_str());

	core::game.must_quit = true;
}

COMMAND_ROUTINE (echo)
{
	if (ev != PRESS)
		return;

	for (const std::string& arg: args)
		std::cout << arg << " ";

	std::cout << std::endl;
}

COMMAND_ROUTINE (bind)
{
	if (ev != PRESS)
		return;
	if (args.size() < 2)
		return;

	std::string keyname = args[0];
	for (char& c: keyname) {
		if (c == '_')
			c = ' ';
	}

	std::string bind;
	for (int i = 1; i < args.size(); i++) {
		bind += args[i];
		bind += ' ';
	}

	key_binds.add_bind(scancode_from_name(keyname),
			parse_command(bind));
}

COMMAND_ROUTINE (stdincmd)
{
	if (ev != PRESS)
		return;
	std::string line;
	std::cout << "Command:" << std::endl;
	std::getline(std::cin, line);
	cmd_registry.run(parse_command(line), PRESS);
}

COMMAND_ROUTINE (exec)
{
	if (ev != PRESS)
		return;
	for (const std::string& s: args)
		run_script(s);
}


COMMAND_ROUTINE (move)
{
	if (args.empty())
		return;

	auto& flags = render::cam_move_flags;
	bool f = (ev == PRESS);

	switch (tolower(args[0][0])) {
	case 'f':
		flags[render::cam_move_f] = f;
		break;
	case 'b':
		flags[render::cam_move_b] = f;
		break;
	case 'l':
		flags[render::cam_move_l] = f;
		break;
	case 'r':
		flags[render::cam_move_r] = f;
		break;
	}
}

MOUSEMOVE_ROUTINE (mousemove_camera)
{
	render::camera.ang.x += dy;
	render::camera.ang.z += dx;
}

}
}

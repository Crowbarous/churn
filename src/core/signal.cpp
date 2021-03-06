#include "signal.h"
#include "input/cmds.h"
#include "core.h"
#include "entity.h"

std::priority_queue<t_signal> signals;
bool operator< (const t_signal& a, const t_signal& b)
{
	return a.tick_due > b.tick_due;
}

void t_signal::execute () const
{
	e_base* e = ents.find_by_name(target);
	if (e == nullptr) {
		warning("tried to send signal to nonexistent entity %s",
				target.c_str());
		return;
	}

	const t_sigmap& sigmap = e->get_sigmap();
	auto i = sigmap.find(signal_name);

	if (i == sigmap.end())
		return;

	f_sig_handler routine = i->second;
	if (routine != nullptr)
		routine(e, argument);
}

void add_signal (t_signal s)
{
	s.tick_due += tick;
	signals.push(s);
}

COMMAND_ROUTINE (signal)
{
	if (ev != PRESS)
		return;

	if (args.size() < 3)
		return;

	int delay = atoi(args[0].c_str());
	const std::string& recipient = args[1];
	const std::string& signame = args[2];

	std::string sigarg;
	if (args.size() >= 4) {
		for (unsigned int i = 3; i < args.size(); i++) {
			sigarg += args[i];
			sigarg += " ";
		}
		sigarg.pop_back();
	}
	t_signal s = { recipient, delay, signame, sigarg };
	add_signal(s);
}

SIG_HANDLER (base, setpos)
{
	atovec3(arg, ent->pos);
	ent->moved();
}

SIG_HANDLER (base, addpos)
{
	ent->pos += atovec3(arg);
	ent->moved();
}

SIG_HANDLER (base, setang)
{
	atovec3(arg, ent->ang);
	ent->moved();
}

SIG_HANDLER (base, setname)
{
	ent->set_name(arg);
}

SIG_HANDLER (base, showpos)
{
	std::ostringstream os;
	os << ent->name
	   << " - pos " << ent->pos
	   << " ang " << ent->ang << '\n';
	DEBUG_MSG(os.str());
}

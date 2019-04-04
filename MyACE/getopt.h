#pragma once

#include "ace/ACE.h"
#include "ace/Get_Opt.h"
#include "ace/OS_NS_strings.h"
#include "ace/OS.h"
#include "ace/ARGV.h"

int testOpt()
{
	static const ACE_TCHAR cmdline[] = ACE_TEXT("-f /home/alem.cfg -h $ACE_ROOT");
	static const ACE_TCHAR options[] = ACE_TEXT("f:h:");
	ACE_ARGV cmdline_args(cmdline, true);
	ACE_Get_Opt cmd_opts(cmdline_args.argc(), cmdline_args.argv(), options, 0);
	int ret = cmd_opts.long_option(ACE_TEXT("config"), 'f', ACE_Get_Opt::ARG_REQUIRED);
	if (-1 == ret)
		return -1;

	int option;
	ACE_TCHAR configfile[MAXPATHLEN];
	ACE_TCHAR hostname[MAXPATHLEN];
	ACE_OS_String::strcpy (configfile, ACE_TEXT("HAStatus.conf"));
	while ((option = cmd_opts()) != EOF)
	{
		switch (option)
		{
		case 'f':
			ACE_OS_String::strncpy(configfile, cmd_opts.opt_arg(), MAXPATHLEN);
			break;
		case 'h':
			ACE_OS_String::strncpy(hostname, cmd_opts.opt_arg(), MAXPATHLEN);
			break;
		case ':':
			ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("-%c requires an argument.\n"), cmd_opts.opt_opt()), -1);
			break;
		default:
			ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("Parse error.\n")), -1);
			break;
		}
	}

	ACE_DEBUG((LM_INFO, configfile));
	ACE_DEBUG((LM_INFO, ACE_TEXT("\n")));
	ACE_DEBUG((LM_INFO, hostname, ACE_TEXT("\n")));
	ACE_DEBUG((LM_INFO, ACE_TEXT("\n")));

	return 0;
}
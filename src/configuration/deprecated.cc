/*
** Copyright 2015 Merethis
**
** This file is part of Centreon Engine.
**
** Centreon Engine is free software: you can redistribute it and/or
** modify it under the terms of the GNU General Public License version 2
** as published by the Free Software Foundation.
**
** Centreon Engine is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
** General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with Centreon Engine. If not, see
** <http://www.gnu.org/licenses/>.
*/

#include "com/centreon/engine/configuration/deprecated.hh"

using namespace com::centreon::engine::configuration;

char const* const deprecated::auto_reschedule_msg =
  "         Centreon Engine embeds an intelligent scheduling engine\n"
  "         which automatically reschedule tasks as its running to smooth\n"
  "         local and remote load\n";
char const* const deprecated::check_acceptance_msg =
  "         Active checks cannot be disabled globally (only on a per\n"
  "         host/service basis) and passive checks are always accepted\n"
  "         when submitted.\n";
char const* const deprecated::check_result_path_msg =
  "         Centreon Engine uses in-memory data structure to feed the\n"
  "         check execution loop with check results. This is far more\n"
  "         efficient than using on-disk files. If you wish to feed\n"
  "         Centreon Engine with your own check result, use the external\n"
  "         command module.\n";
char const* const deprecated::default_msg =
  "         This feature is not available in Centreon Engine.\n";
char const* const deprecated::embedded_perl_msg =
  "         Centreon Engine does not have any Perl interpreter embedded.\n"
  "         It instead relies on external software to provide fast Perl\n"
  "         script execution (using precompilation). An example of such\n"
  "         software is Centreon Connector Perl.\n";
char const* const deprecated::engine_performance_msg =
  "         Centreon Engine is just that powerful that it does not need\n"
  "         this feature.\n";
char const* const deprecated::external_command_msg =
  "         External commands are handled by a module. If you do not\n"
  "         wish to check for external commands, do not load the module.\n";
char const* const deprecated::groups_msg =
  "         Groups are not supported. As a consequence all related\n"
  "         configuration properties were removed along with host groups,\n"
  "         service groups and contact groups objects. You should use\n"
  "         Centreon to easily configure your monitoring infrastructure\n"
  "         (among other cool features).\n";
char const* const deprecated::interval_length_msg =
  "         Time units cannot be set globally. All properties that\n"
  "         configure a duration (*_interval, *_timeout, ...) can be set\n"
  "         using suffixes. Known suffixes are 's' for seconds (the default\n"
  "         if omitted), 'm' for minutes, 'h' for hours and 'd' for days.\n";
char const* const deprecated::log_msg =
  "         Log rotation should be handled by external software (such as\n"
  "         logrotate, for which Centreon Engine provides a sample script).\n";
char const* const deprecated::notification_msg =
  "         Centreon Engine does not have any notification engine. As\n"
  "         a consequence all related configuration properties were\n"
  "         removed along with as contacts, contact groups, escalations,\n"
  "         acknowledgements and scheduled downtime objects. You should\n"
  "         use Centreon Broker and its notification module if you\n"
  "         wish to send notifications.\n";
char const* const deprecated::passive_host_checks_msg =
  "         All passive check results are treated the same way than active\n"
  "         check results.\n";
char const* const deprecated::perfdata_msg =
  "         Performance data are not processed by Centreon Engine.\n"
  "         In a Centreon environment, graphs are generated by Centreon\n"
  "         Broker and vizualized using Centreon.\n";
char const* const deprecated::retention_usage_msg =
  "         All runtime data used by Centreon Engine is dumped to the\n"
  "         retention file at regular intervals and at shutdown. The\n"
  "         content of the file is read back when starting. All this is\n"
  "         done automatically and do not need cumbersome configuration.\n";
char const* const deprecated::startup_script_msg =
  "         This feature should be handled by the startup script.\n"
  "         Check it out and modify it to suite your needs.\n";
char const* const deprecated::status_file_usage_msg =
  "         The status file contains all objects statuses and is\n"
  "         written only in response to the SAVE_STATUS_INFORMATION\n"
  "         external command.\n";
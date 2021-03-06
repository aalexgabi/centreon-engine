/*
** Copyright 2011-2015 Merethis
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

#include <string>
#include "com/centreon/engine/config.hh"
#include "com/centreon/engine/configuration/applier/state.hh"
#include "com/centreon/engine/configuration/parser.hh"
#include "com/centreon/engine/configuration/state.hh"
#include "com/centreon/engine/deleter.hh"
#include "com/centreon/engine/error.hh"
#include "com/centreon/engine/globals.hh"
#include "com/centreon/engine/macros.hh"
#include "com/centreon/engine/objects/hostdependency.hh"
#include "com/centreon/engine/objects/servicedependency.hh"
#include "com/centreon/engine/retention/parser.hh"
#include "com/centreon/engine/retention/state.hh"
#include "com/centreon/engine/string.hh"
#include "com/centreon/shared_ptr.hh"
#include "compatibility/locations.h"
#include "chkdiff.hh"
#include "config.hh"
#include "skiplist.h"
#include "test/unittest.hh"
#include "xodtemplate.hh"
#include "xrddefault.hh"

using namespace com::centreon;
using namespace com::centreon::engine;

struct                global {
  command*            commands;
  host*               hosts;
  hostdependency*     hostdependencies;
  service*            services;
  servicedependency*  servicedependencies;
  timeperiod*         timeperiods;

  umap<std::string, shared_ptr<command> >
                      save_commands;
  umap<std::string, shared_ptr<commands::connector> >
                      save_connectors;
  umap<std::string, shared_ptr<host> >
                      save_hosts;
  umultimap<std::string, shared_ptr<hostdependency> >
                      save_hostdependencies;
  umap<std::pair<std::string, std::string>, shared_ptr<service> >
                      save_services;
  umultimap<std::pair<std::string, std::string>, shared_ptr<servicedependency> >
                      save_servicedependencies;
  umap<std::string, shared_ptr<timeperiod> >
                      save_timeperiods;

  int                 additional_freshness_latency;
  unsigned long       cached_host_check_horizon;
  unsigned long       cached_service_check_horizon;
  bool                check_host_freshness;
  unsigned int        check_reaper_interval;
  bool                check_service_freshness;
  int                 command_check_interval;
  std::string         command_file;
  std::string         debug_file;
  // unsigned long       debug_level;
  unsigned int        debug_verbosity;
  bool                enable_event_handlers;
  bool                enable_flap_detection;
  bool                enable_predictive_host_dependency_checks;
  bool                enable_predictive_service_dependency_checks;
  unsigned long       event_broker_options;
  unsigned int        event_handler_timeout;
  int                 external_command_buffer_slots;
  std::string         global_host_event_handler;
  std::string         global_service_event_handler;
  float               high_host_flap_threshold;
  float               high_service_flap_threshold;
  unsigned int        host_check_timeout;
  unsigned int        host_freshness_check_interval;
  std::string         illegal_object_chars;
  std::string         illegal_output_chars;
  bool                log_event_handlers;
  bool                log_external_commands;
  //  std::string         log_file;
  bool                log_host_retries;
  bool                log_initial_states;
  bool                log_passive_checks;
  bool                log_service_retries;
  float               low_host_flap_threshold;
  float               low_service_flap_threshold;
  unsigned long       max_debug_file_size;
  unsigned int        max_parallel_service_checks;
  bool                obsess_over_hosts;
  bool                obsess_over_services;
  std::string         ochp_command;
  unsigned int        ochp_timeout;
  std::string         ocsp_command;
  unsigned int        ocsp_timeout;
  unsigned int        retention_update_interval;
  unsigned int        service_check_timeout;
  unsigned int        service_freshness_check_interval;
  float               sleep_time;
  bool                soft_state_dependencies;
  unsigned int        time_change_threshold;
  bool                use_syslog;
  std::string         use_timezone;
};

#define check_value(id) \
  if (g1.id != g2.id) { \
    std::cerr << #id << " is different (" \
      << g1.id << ", " << g2.id << ")" << std::endl; \
    ret = false; \
  }

static std::string to_str(char const* str) { return (str ? str : ""); }

template<typename T>
static void reset_next_check(T* lst) {
  for (T* obj(lst); obj; obj = obj->next) {
    obj->next_check = 0;
    obj->should_be_scheduled = 1;
  }
}

/**
 *  Sort a list.
 */
template <typename T>
static void sort_it(T*& l) {
  T* remaining(l);
  T** new_root(&l);
  *new_root = NULL;
  while (remaining) {
    T** min(&remaining);
    for (T** cur(&((*min)->next)); *cur; cur = &((*cur)->next))
      if (**cur < **min)
        min = cur;
    *new_root = *min;
    *min = (*min)->next;
    new_root = &((*new_root)->next);
    *new_root = NULL;
  }
  return ;
}

/**
 *  Sort a list.
 */
template <typename T>
static void sort_it_rev(T*& l) {
  T* remaining(l);
  T** new_root(&l);
  *new_root = NULL;
  while (remaining) {
    T** min(&remaining);
    for (T** cur(&((*min)->next)); *cur; cur = &((*cur)->next))
      if (!(**cur < **min))
        min = cur;
    *new_root = *min;
    *min = (*min)->next;
    new_root = &((*new_root)->next);
    *new_root = NULL;
  }
  return ;
}

/**
 *  Remove duplicate members of a list.
 */
template <typename T>
static void remove_duplicates(T* l) {
  while (l) {
    for (T* m(l->next); m; m = m->next)
      if (*l == *m)
        l->next = m->next;
    l = l->next;
  }
  return ;
}

/**
 *  Check difference between global object.
 *
 *  @param[in] l1 The first struct.
 *  @param[in] l2 The second struct.
 *
 *  @return True if globals are equal, otherwise false.
 */
bool chkdiff(global& g1, global& g2) {
  bool ret(true);

  check_value(additional_freshness_latency);
  check_value(cached_host_check_horizon);
  check_value(cached_service_check_horizon);
  check_value(check_host_freshness);
  check_value(check_reaper_interval);
  check_value(check_service_freshness);
  check_value(command_check_interval);
  check_value(command_file);
  check_value(debug_file);
  // check_value(debug_level);
  check_value(debug_verbosity);
  check_value(enable_event_handlers);
  check_value(enable_flap_detection);
  check_value(enable_predictive_host_dependency_checks);
  check_value(enable_predictive_service_dependency_checks);
  check_value(event_broker_options);
  check_value(event_handler_timeout);
  check_value(external_command_buffer_slots);
  check_value(global_host_event_handler);
  check_value(global_service_event_handler);
  check_value(high_host_flap_threshold);
  check_value(high_service_flap_threshold);
  check_value(host_check_timeout);
  check_value(host_freshness_check_interval);
  check_value(illegal_object_chars);
  check_value(illegal_output_chars);
  check_value(log_event_handlers);
  check_value(log_external_commands);
  // check_value(log_file);
  check_value(log_host_retries);
  check_value(log_initial_states);
  check_value(log_passive_checks);
  check_value(log_service_retries);
  check_value(low_host_flap_threshold);
  check_value(low_service_flap_threshold);
  check_value(max_debug_file_size);
  check_value(max_parallel_service_checks);
  check_value(obsess_over_hosts);
  check_value(obsess_over_services);
  check_value(ochp_command);
  check_value(ochp_timeout);
  check_value(ocsp_command);
  check_value(ocsp_timeout);
  check_value(retention_update_interval);
  check_value(service_check_timeout);
  check_value(service_freshness_check_interval);
  check_value(sleep_time);
  check_value(soft_state_dependencies);
  check_value(time_change_threshold);
  check_value(use_syslog);
  check_value(use_timezone);

  if (!chkdiff(g1.commands, g2.commands))
    ret = false;
  reset_next_check(g1.hosts);
  reset_next_check(g2.hosts);
  for (host_struct* h(g1.hosts); h; h = h->next)
    sort_it(h->parent_hosts);
  for (host_struct* h(g2.hosts); h; h = h->next)
    sort_it(h->parent_hosts);
  if (!chkdiff(g1.hosts, g2.hosts))
    ret = false;
  sort_it(g1.hostdependencies);
  remove_duplicates(g1.hostdependencies);
  sort_it(g2.hostdependencies);
  remove_duplicates(g2.hostdependencies);
  if (!chkdiff(g1.hostdependencies, g2.hostdependencies))
    ret = false;
  reset_next_check(g1.services);
  reset_next_check(g2.services);
  for (service_struct* s(g1.services); s; s = s->next)
    sort_it_rev(s->custom_variables);
  if (!chkdiff(g1.services, g2.services))
    ret = false;
  sort_it(g1.servicedependencies);
  remove_duplicates(g1.servicedependencies);
  sort_it(g2.servicedependencies);
  remove_duplicates(g2.servicedependencies);
  if (!chkdiff(g1.servicedependencies, g2.servicedependencies))
    ret = false;
  if (!chkdiff(g1.timeperiods, g2.timeperiods))
    ret = false;
  return (ret);
}

/**
 *  Get all global configuration.
 *
 *  @retrun All global.
 */
static global get_globals() {
  global g;
  g.commands = command_list;
  command_list = NULL;
  g.hosts = host_list;
  host_list = NULL;
  g.hostdependencies = hostdependency_list;
  hostdependency_list = NULL;
  g.services = service_list;
  service_list = NULL;
  g.servicedependencies = servicedependency_list;
  servicedependency_list = NULL;
  g.timeperiods = timeperiod_list;
  timeperiod_list = NULL;

  configuration::applier::state&
    app_state(configuration::applier::state::instance());
  g.save_commands = app_state.commands();
  app_state.commands().clear();
  g.save_connectors = app_state.connectors();
  app_state.connectors().clear();
  g.save_hosts = app_state.hosts();
  app_state.hosts().clear();
  g.save_hostdependencies = app_state.hostdependencies();
  app_state.hostdependencies().clear();
  g.save_services = app_state.services();
  app_state.services().clear();
  g.save_servicedependencies = app_state.servicedependencies();
  app_state.servicedependencies().clear();
  g.save_timeperiods = app_state.timeperiods();
  app_state.timeperiods().clear();

  g.additional_freshness_latency = additional_freshness_latency;
  g.cached_host_check_horizon = cached_host_check_horizon;
  g.cached_service_check_horizon = cached_service_check_horizon;
  g.check_host_freshness = check_host_freshness;
  g.check_reaper_interval = check_reaper_interval;
  g.check_service_freshness = check_service_freshness;
  g.command_check_interval = command_check_interval;
  g.command_file = to_str(command_file);
  g.debug_file = to_str(debug_file);
  // g.debug_level = debug_level;
  g.debug_verbosity = debug_verbosity;
  g.enable_event_handlers = enable_event_handlers;
  g.enable_flap_detection = enable_flap_detection;
  g.enable_predictive_host_dependency_checks = enable_predictive_host_dependency_checks;
  g.enable_predictive_service_dependency_checks = enable_predictive_service_dependency_checks;
  g.event_broker_options = event_broker_options;
  g.event_handler_timeout = event_handler_timeout;
  g.external_command_buffer_slots = external_command_buffer_slots;
  g.global_host_event_handler = to_str(global_host_event_handler);
  g.global_service_event_handler = to_str(global_service_event_handler);
  g.high_host_flap_threshold = high_host_flap_threshold;
  g.high_service_flap_threshold = high_service_flap_threshold;
  g.host_check_timeout = host_check_timeout;
  g.host_freshness_check_interval = host_freshness_check_interval;
  g.illegal_object_chars = to_str(illegal_object_chars);
  g.illegal_output_chars = to_str(illegal_output_chars);
  g.log_event_handlers = log_event_handlers;
  g.log_external_commands = log_external_commands;
  // g.log_file = to_str(log_file);
  g.log_host_retries = log_host_retries;
  g.log_initial_states = log_initial_states;
  g.log_passive_checks = log_passive_checks;
  g.log_service_retries = log_service_retries;
  g.low_host_flap_threshold = low_host_flap_threshold;
  g.low_service_flap_threshold = low_service_flap_threshold;
  g.max_debug_file_size = max_debug_file_size;
  g.max_parallel_service_checks = max_parallel_service_checks;
  g.obsess_over_hosts = obsess_over_hosts;
  g.obsess_over_services = obsess_over_services;
  g.ochp_command = to_str(ochp_command);
  g.ochp_timeout = ochp_timeout;
  g.ocsp_command = to_str(ocsp_command);
  g.ocsp_timeout = ocsp_timeout;
  g.retention_update_interval = retention_update_interval;
  g.service_check_timeout = service_check_timeout;
  g.service_freshness_check_interval = service_freshness_check_interval;
  g.sleep_time = sleep_time;
  g.soft_state_dependencies = soft_state_dependencies;
  g.time_change_threshold = time_change_threshold;
  g.use_syslog = use_syslog;
  g.use_timezone = to_str(use_timezone);
  return (g);
}

/**
 *  Read configuration with new parser.
 *
 *  @parser[out] g        Fill global variable.
 *  @param[in]  filename  The file path to parse.
 *  @parse[in]  options   The options to use.
 *
 *  @return True on succes, otherwise false.
 */
static bool newparser_read_config(
              global& g,
              std::string const& filename,
              unsigned int options) {
  bool ret(false);
  try {
    init_macros();
    // Parse configuration.
    configuration::state config;

    // tricks to bypass create log file.
    config.log_file("");

    {
      configuration::parser p(options);
      p.parse(filename, config);
    }

    // Parse retention.
    retention::state state;
    try {
      retention::parser p;
      p.parse(config.state_retention_file(), state);
    }
    catch (...) {
    }

    configuration::applier::state::instance().apply(config, state);

    g = get_globals();
    clear_volatile_macros_r(get_global_macros());
    free_macrox_names();
    ret = true;
  }
  catch (std::exception const& e) {
    std::cerr << e.what() << std::endl;
  }
  return (ret);
}

/**
 *  Read configuration with old parser.
 *
 *  @parser[out] g        Fill global variable.
 *  @param[in]   filename The file path to parse.
 *  @parse[in]   options  The options to use.
 *
 *  @return True on succes, otherwise false.
 */
static bool oldparser_read_config(
              global& g,
              std::string const& filename,
              unsigned int options) {
  xrddefault_initialize_retention_data(filename.c_str());
  clear_volatile_macros_r(get_global_macros());
  free_macrox_names();
  init_object_skiplists();
  init_macros();
  int ret(read_main_config_file(filename.c_str()));
  if (ret == OK)
    ret = xodtemplate_read_config_data(filename.c_str(), options);
  if (ret == OK)
    ret = pre_flight_check();
  if (!command_file)
    command_file = string::dup(DEFAULT_COMMAND_FILE);
  if (!debug_file)
    debug_file = string::dup(DEFAULT_DEBUG_FILE);
  if (!illegal_output_chars)
    illegal_output_chars = string::dup("`~$&|'\"<>");
  if (ret == OK) {
    xrddefault_read_state_information();
    g = get_globals();
  }
  clear_volatile_macros_r(get_global_macros());
  free_macrox_names();
  free_object_skiplists();
  xrddefault_cleanup_retention_data(filename.c_str());
  return (ret == OK);
}

/**
 *  Check if the configuration parser works properly.
 *
 *  @return 0 on success.
 */
int main_test(int argc, char** argv) {
  if (argc != 2)
    throw (engine_error() << "usage: " << argv[0] << " file.cfg");

  unsigned int options(configuration::parser::read_all);

  global oldcfg;
  if (!oldparser_read_config(oldcfg, argv[1], options))
    throw (engine_error() << "old parser can't parse " << argv[1]);

  global newcfg;
  if (!newparser_read_config(newcfg, argv[1], options))
    throw (engine_error() << "new parser can't parse " << argv[1]);

  bool ret(chkdiff(oldcfg, newcfg));

  return (!ret);
}

/**
 *  Init unit test.
 */
int main(int argc, char** argv) {
  unittest utest(argc, argv, &main_test);
  return (utest.run());
}

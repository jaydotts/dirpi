#!/bin/bash
#
#
#
# Collection of protection protocols for dirpi commands. Includes boilerplate methods for: 
# - Running function with a timeout 
#

run_with_timeout() {
  local timeout=$1
  shift
  local command=("$@")

  # Start the command in the background
  "${command[@]}" &

  # Get the process ID (PID) of the command
  local command_pid=$!

  # Sleep for the specified timeout period
  sleep "$timeout"

  # Check if the command is still running
  if ps -p "$command_pid" > /dev/null; then
    # Kill the command process
    kill "$command_pid"
    echo "Command killed after $timeout seconds"
  else
    echo "Command completed before timeout"
  fi
}
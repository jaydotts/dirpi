#!/bin/bash
#
#
#
# Collection of helper methods for dirpi commands. Includes boilerplate methods for: 
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

parse_config() {
  local config_file=$1
  local keyword=$2
  local default=$3

  # Read the config file line by line
  while IFS='=' read -r key value; do
    # Remove leading/trailing whitespace from the key and value
    key=${key// /}
    value=${value// /}
    
    # Check if the current line contains the desired keyword
    if [[ "$key" == "$keyword" ]]; then
      echo "$value"
      return
    fi
  done < "$config_file"

  echo $default
}
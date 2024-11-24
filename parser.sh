#!/bin/bash

# Input log file path
# Name can change based on the log file you want to parse, but every logfile ends with .log and is in the folder data

# Find the latest log file in the data directory
LOGFILE=$(ls -t ./data/*.log | head -1)

LOGFILE_PATH=$LOGFILE
# Output JSON file path
OUTPUT_JSON_PATH="./data/parsed_logs.json"

# Check if log file exists
if [ ! -f "$LOGFILE_PATH" ]; then
  echo "Log file not found: $LOGFILE_PATH"
  exit 1
fi

# Create an empty JSON array to store parsed logs
echo "[" > "$OUTPUT_JSON_PATH"

# Read log file line by line
FIRST_LINE=true
while IFS= read -r line; do
  # Parse each line as JSON and append to output file
  if $FIRST_LINE; then
    FIRST_LINE=false
  else
    echo "," >> "$OUTPUT_JSON_PATH"
  fi
  echo "$line" >> "$OUTPUT_JSON_PATH"
done < "$LOGFILE_PATH"

# Close the JSON array
echo "]" >> "$OUTPUT_JSON_PATH"

# Print completion message
echo "Parsed logs saved to: $OUTPUT_JSON_PATH"

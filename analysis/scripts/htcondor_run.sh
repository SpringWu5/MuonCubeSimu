#!/bin/bash
set -euo pipefail

# Extract arguments
GITHUB_TOKEN="$1"
GITHUB_REPO="$2"
PR_NUMBER="$3"
COMMIT_SHA="$4"
JOB_ID="$5"
RELEASE_URL="$6"  # Optional parameter

# Get the absolute path of the repository directory
REPO_DIR_ABS=$(pwd)

echo "Starting HTCondor job for commit: $COMMIT_SHA"

# Run Simulation
echo "Starting simulation..."
./simulation/SLabSimu

# Determine the output filename from the config
OUTPUT_FILE=$(grep -A 1 "filename:" simulation/config/config.yaml | grep -v "filename:" | xargs)
if [ -z "$OUTPUT_FILE" ]; then
    OUTPUT_FILE="output.root"  # default fallback
fi

# Run Basic Validation
echo "Simulation finished. Starting validation..."
python analysis/scripts/standard_validation.py \
  --input-file "$OUTPUT_FILE" \
  --output-dir "results"

# Archive Primary Data
echo "Creating data archive..."
zip MuonCubeSimu_output_${COMMIT_SHA}.zip "$OUTPUT_FILE"

# Create GitHub Release (only if not already created)
if [ -z "${RELEASE_URL:-}" ]; then
  echo "Creating GitHub release..."
  gh release create "v${COMMIT_SHA}" \
      --repo "$GITHUB_REPO" \
      --title "Data Release for ${COMMIT_SHA}" \
      --notes "Automated data release for PR #${PR_NUMBER}, Commit: ${COMMIT_SHA}" \
      "MuonCubeSimu_output_${COMMIT_SHA}.zip#Simulation Output Data"
  RELEASE_URL="https://github.com/$GITHUB_REPO/releases/tag/v${COMMIT_SHA}"
fi

# Run Post-Processing (reads manifest and reports back)
echo "Validation and release finished. Reporting results to GitHub..."
python analysis/scripts/post_process.py \
  --manifest "results/results_manifest.json" \
  --github-token "$GITHUB_TOKEN" \
  --repo "$GITHUB_REPO" \
  --pr-number "$PR_NUMBER" \
  --commit-sha "$COMMIT_SHA" \
  --job-id "$JOB_ID" \
  --release-url "$RELEASE_URL"

echo "HTCondor job completed successfully."
#!/usr/bin/env python3
import argparse
import json
import os
import subprocess
import sys
from pathlib import Path

def run_command(command, check=True):
    """Executes a command and returns its output."""
    print(f"Executing: {' '.join(command)}", flush=True)
    try:
        result = subprocess.run(
            command,
            check=check,
            capture_output=True,
            text=True
        )
        print(result.stdout, flush=True)
        if result.stderr:
            print(f"Stderr: {result.stderr}", file=sys.stderr, flush=True)
        return result.stdout.strip()
    except subprocess.CalledProcessError as e:
        print(f"Error executing command: {' '.join(command)}", file=sys.stderr, flush=True)
        print(f"Return code: {e.returncode}", file=sys.stderr, flush=True)
        print(f"Output:\n{e.stdout}", file=sys.stderr, flush=True)
        print(f"Stderr:\n{e.stderr}", file=sys.stderr, flush=True)
        raise

def create_markdown_report(manifest):
    """Creates a markdown report from the manifest."""
    report = []
    metadata = manifest.get("run_metadata", {})
    summary = manifest.get("summary", {})
    artifacts = manifest.get("artifacts", [])

    pr_number = metadata.get("pr_number")
    commit_sha = metadata.get("commit_sha")
    run_size = metadata.get("run_size", "N/A")
    job_id = metadata.get("job_id", "N/A")

    report.append(f"### 🔬 Simulation Results: **{run_size.capitalize()} Run**")
    report.append(f"**PR:** `#{pr_number}` | **Commit:** `{commit_sha[:7]}` | **Job ID:** `{job_id}`")
    report.append("---")

    if summary.get("title") and summary.get("data"):
        report.append(f"#### {summary['title']}")
        report.append("| Metric | Value |")
        report.append("|--------|-------|")
        for key, value in summary["data"].items():
            report.append(f"| {key} | {value} |")
        report.append("")

    key_artifacts = [art for art in artifacts if art.get("is_key_result")]
    if key_artifacts:
        report.append("#### Key Results")
        for art in key_artifacts:
            # Placeholder for image URL, will be replaced after release upload
            report.append(f"##### {art.get('title', 'Untitled')}")
            report.append(f"![{art.get('title', 'image')}]({art.get('path')})")
            report.append(f"_{art.get('description', '')}_")
        report.append("")
    
    report.append("---")
    report.append("> A complete archive of all generated files is available in the created GitHub Release.")

    return "\n".join(report)


def main():
    parser = argparse.ArgumentParser(description="Analyze simulation results and report them to GitHub.")
    parser.add_argument("--manifest", required=True, help="Path to the results_manifest.json file.")
    parser.add_argument("--github-token", required=True, help="GitHub PAT for authentication.")
    parser.add_argument("--repo", required=True, help="GitHub repository in 'owner/repo' format.")
    parser.add_argument("--pr-number", required=True, help="The Pull Request number.")
    parser.add_argument("--commit-sha", required=True, help="The commit SHA the run was based on.")
    parser.add_argument("--job-id", default="N/A", help="The cluster job ID, if applicable.")
    args = parser.parse_args()

    # --- 1. Set up environment for gh CLI ---
    os.environ['GITHUB_TOKEN'] = args.github_token
    run_command(["gh", "auth", "status"])

    # --- 2. Load and parse the manifest file ---
    manifest_path = Path(args.manifest)
    if not manifest_path.is_file():
        print(f"Error: Manifest file not found at {manifest_path}", file=sys.stderr)
        sys.exit(1)
    
    with open(manifest_path, 'r') as f:
        manifest = json.load(f)

    results_dir = manifest_path.parent
    
    # --- 3. Package all artifacts into a zip file ---
    zip_file_path = results_dir / "results.zip"
    artifacts_to_zip = [art["path"] for art in manifest.get("artifacts", [])]
    # Also include the manifest itself for completeness
    artifacts_to_zip.append(manifest_path.name)

    zip_command = ["zip", str(zip_file_path)] + artifacts_to_zip
    run_command(zip_command, check=True)

    # --- 4. Create a unique tag and create the GitHub Release ---
    tag_name = f"exp/{args.pr_number}/{args.commit_sha[:7]}"
    release_title = f"Experimental Run for PR #{args.pr_number} ({args.commit_sha[:7]})")"
    release_notes = f"Full data archive for simulation run based on commit {args.commit_sha}."

    print(f"Creating release with tag: {tag_name}")
    # Use --generate-notes to add a list of commits since last release
    release_command = [
        "gh", "release", "create", tag_name, str(zip_file_path),
        "--repo", args.repo,
        "--title", release_title,
        "--notes", release_notes,
        "--target", args.commit_sha,
        "--generate-notes"
    ]
    release_url = run_command(release_command)
    print(f"Successfully created release: {release_url}")

    # --- 5. Generate and post the PR comment ---
    # Note: For simplicity, this version uses local paths for images.
    # A more advanced version would parse the release_url to get asset URLs.
    report_md = create_markdown_report(manifest)
    
    # Add a link to the release at the end of the report
    final_report_md = f"{report_md}\n\n**[View Full Results Archive in Release]({release_url})**"

    report_file_path = results_dir / "report.md"
    with open(report_file_path, 'w') as f:
        f.write(final_report_md)

    print("Posting comment to PR...")
    comment_command = [
        "gh", "pr", "comment", args.pr_number,
        "--repo", args.repo,
        "--body-file", str(report_file_path)
    ]
    run_command(comment_command)

    print("--- Post-processing complete. ---")

if __name__ == "__main__":
    main()

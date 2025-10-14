# <legal>
# Pointer Ownership Model (POM) Source Code Release
# 
# Copyright 2025 Carnegie Mellon University.
# 
# NO WARRANTY. THIS CARNEGIE MELLON UNIVERSITY AND SOFTWARE ENGINEERING
# INSTITUTE MATERIAL IS FURNISHED ON AN "AS-IS" BASIS. CARNEGIE MELLON
# UNIVERSITY MAKES NO WARRANTIES OF ANY KIND, EITHER EXPRESSED OR
# IMPLIED, AS TO ANY MATTER INCLUDING, BUT NOT LIMITED TO, WARRANTY OF
# FITNESS FOR PURPOSE OR MERCHANTABILITY, EXCLUSIVITY, OR RESULTS
# OBTAINED FROM USE OF THE MATERIAL. CARNEGIE MELLON UNIVERSITY DOES NOT
# MAKE ANY WARRANTY OF ANY KIND WITH RESPECT TO FREEDOM FROM PATENT,
# TRADEMARK, OR COPYRIGHT INFRINGEMENT.
# 
# Licensed under a MIT (SEI)-style license, please see license.txt or
# contact permission@sei.cmu.edu for full terms.
# 
# [DISTRIBUTION STATEMENT A] This material has been approved for public
# release and unlimited distribution.  Please see Copyright notice for
# non-US Government use and distribution.
# 
# DM25-1262
# </legal>


import sys
import asyncio
from pathlib import Path

import aiofiles               # pip install aiofiles
from openai import AsyncOpenAI # pip install --upgrade openai

import json

# -------------------------- core worker -------------------------- #
def valid_response(reply_path: Path) -> bool:
    if not reply_path.exists():
        return False
    with reply_path.open('r', encoding='utf-8') as f:
        try:
            wisdom = json.load(f)
            return True
        except Exception as e:
            return False


async def process_query_file(
    query_path: Path,
    client: AsyncOpenAI,
    model: str = "o4-mini-2025-04-16",
) -> None:
    """Read <file>.query → call OpenAI → write <file>.reply (async)."""
    reply_path = query_path.with_suffix(".reply")
    if valid_response(reply_path):
        print(f"Skipping '{query_path}': reply already exists.")
        return

    # --- read prompt ------------------------------------------------
    try:
        async with aiofiles.open(query_path, "r", encoding="utf-8") as f:
            prompt = await f.read()
    except Exception as e:
        print(f"Error reading '{query_path}': {e}")
        return

    for index in range(3):

        # --- call OpenAI ------------------------------------------------
        try:
            completion = await client.chat.completions.create(
                model=model,
                messages=[{"role": "user", "content": prompt}],
            )
            response = completion.choices[0].message.content
        except Exception as e:
            print(f"Error generating completion for '{query_path}': {e}")
            return

        # --- write reply -----------------------------------------------
        try:
            if not response.endswith("\n"):
                response += "\n"
            async with aiofiles.open(reply_path, "w", encoding="utf-8") as f:
                await f.write(response)
            print(f"Wrote reply to '{reply_path}'.")
        except Exception as e:
            print(f"Error writing '{reply_path}': {e}")

        if valid_response(reply_path):
            print(f"Valid response in '{reply_path}'.")
            return

        print(f"Invalid response in '{reply_path}'.")

    print(F"Too many invalid responses, giving up '{reply_path}'.")


# --------------------- orchestration helpers --------------------- #
async def run_tasks(query_files: list[Path], client: AsyncOpenAI, concurrency: int = 100) -> None:
    """
    Launch a task for each file but bound outstanding API calls with a semaphore.
    Increase or lower *concurrency* as needed for your rate-limit.
    """
    sem = asyncio.Semaphore(concurrency)

    async def _bounded_task(p: Path):
        async with sem:
            await process_query_file(p, client)

    await asyncio.gather(*(_bounded_task(p) for p in query_files))


# ------------------------------ CLI ------------------------------ #
async def main() -> None:
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <file_or_directory>")
        sys.exit(1)

    target = Path(sys.argv[1])
    client = AsyncOpenAI()  # respects OPENAI_API_KEY env var

    if target.is_file():
        if target.suffix != ".query":
            print(f"Error: File '{target}' does not end with '.query'.")
            sys.exit(1)
        await process_query_file(target, client)

    elif target.is_dir():
        query_files = [p for p in target.iterdir() if p.suffix == ".query"]
        if not query_files:
            print(f"No .query files found in directory '{target}'.")
            return
        await run_tasks(query_files, client)

    else:
        print(f"Error: '{target}' is neither a file nor a directory.")
        sys.exit(1)


if __name__ == "__main__":
    asyncio.run(main())

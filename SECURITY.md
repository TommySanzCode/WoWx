# Security policy

## Supported code

WoWx is still in development. Security fixes are focused on the current `main`
branch. There are no stable releases or maintained older release branches yet.
Older test packages may not contain fixes made since they were built.

## Reporting a vulnerability

Please report security problems privately through
[Report a vulnerability](https://github.com/TommySanzCode/WoWx/security/advisories/new).
You can also find this under the repository's Security tab.

Don't post passwords, working exploits or details that put other users at risk
in a public issue. If private reporting is temporarily unavailable, ask for a
private security contact in Discussions without describing the vulnerability.

Include what you can:

- The affected commit or build and whether it involves the Xbox client, PC
  tools or local server setup.
- What the problem allows someone to do and what access they need.
- Steps to reproduce it, preferably using a small test case with dummy data.
- Relevant logs or a suggested fix, with credentials and personal paths removed.

Please don't attach original game assets, firmware, private server databases or
other people's information. Test only on systems and accounts you control or
have permission to test.

## What happens next

I'll review the report, ask for any missing details and work out a fix if the
problem is in WoWx. We can use the private advisory to discuss when to publish
the details. This is a small project, so I can't promise a fixed response time
or a paid bounty.

If the problem belongs to an upstream dependency, I'll help identify the right
project to report it to. Ordinary crashes, setup questions and gameplay bugs
without a security impact can use the normal issue forms.

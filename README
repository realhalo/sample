Sample is the missing link in the POSIX-compliant cron-family of task
scheduling tools. Sample monitors files on a user-by-user basis and waits
for their timestamp/filesize to hit a threshold thereby executing a
specified command to handle the files.


GOALS:

	- POSIX-compliant, portable on unix-based systems.
	- Simple, easy to understand syntax and quick to learn.
	- Versatile, capable for use in many situations.
	- Fast, run quickly and restricted if need be.
	- Small, leave a small footprint in memory and execution.
	- Expandable, can grow without changing the core syntax.
	- Secure, as much as possible running arbitrary commands.


INFORMATION:

The best way to understand sample is to view the manual pages for the
following:

man sample	- command-line interface for users, like crontab is to crond.
man samples	- command-line interface for single-instance/spooled sample files.
man sampled	- the daemon that processes users ~/.sample, and related files.
man samputil	- shell-script interface for sampled. (apachectl-based)
man sample.conf	- the configuration for sampled to use.


PROCESSING NOTES:

Sample will only "sample" matched files if the active user has _write_
access to the file and it does not contain non-printable characters, it
will not process the matched file at any level otherwise.

Sample will only process unique users with unique home directories,
this is done on a first-come frist-serve basis.  So, if a user or
home directory is first in the /etc/passwd file (including network
users) and another entry has the same user or home directory that other
user will not be processed at any level.



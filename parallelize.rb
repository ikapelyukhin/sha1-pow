#!/usr/bin/env ruby

# sha1-pow -- compute proof-of-work for a given 64 byte string
# Copyright (c) 2019 Ivan Kapelyukhin
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

require 'digest'

# Poor man's parallelism: forks a number of child processes equal to the number
# of CPUs (as reported by `nproc`).
def get_pow(prefix, difficulty)
	num_cpu_cores = `nproc`.to_i
	raise "Can't get number of CPU cores" unless (num_cpu_cores > 0)

	pids = []
	r, w = IO.pipe
	num_cpu_cores.times do
		pids << Process.spawn("./sha1-pow #{prefix} #{difficulty}", :out => w, :err => IO::NULL)
	end
	w.close

	Process.waitpid(-1, 0)
	pow = r.gets.strip
	r.close

	pids.each do |pid|
		begin
			Process.kill(9, pid)
		rescue Errno::ESRCH
		end
	end

	pow
end

prefix = ARGV[0]
difficulty = ARGV[1]
raise "Usage: #{$0} <prefix> <difficulty>" unless (prefix && difficulty)

pow = get_pow(prefix, difficulty)
puts pow
warn "Full string: #{prefix + pow}"
warn "SHA1 digest: #{Digest::SHA1.hexdigest(prefix + pow)}"

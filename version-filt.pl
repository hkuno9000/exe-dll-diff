# $_ = "M update-rev commit-rev username filefullpath"
$_ = `svn status -v $ARGV[0]`;
print "svn revision: $2" if /(\d+) +(\d+) (\w+)/;

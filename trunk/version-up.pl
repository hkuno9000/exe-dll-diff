# get svn revision
$_ = `svn status -v Makefile`; # "M update-rev commit-rev username filefullpath"
($updateRev,$commitRev,$username) = ($1,$2,$3) if /(\d+) +(\d+) (\w+)/;

# get version
$major = 0;
$minor = -1;
$versrc = shift;
open(IN, $versrc);
while (<IN>) {
	($major,$minor) = ($1,$2) if (/version-(\d+)\.(\d+)/ && $1 >= $major && $2 > $minor);
}
close(IN);

# update revision
++$updateRev;

print "versrc:$versrc, ver: $major.$minor, rev:$updateRev,$commitRev\n";

# set version
@ARGV=glob("@ARGV");
print "argv:@ARGV\n";
while (<>) {
	s/version \d+\.\d+ \(r\d+\)/version $major.$minor (r$updateRev)/;
	s/\d+,\d+,\d+,\d+/$major,$minor,0,$updateRev/ if /(FILE|PRODUCT)VERSION/;
	s/\d+, \d+, \d+, \d+/$major, $minor, 0, $updateRev/ if /(File|Product)Version/;
	print;
}

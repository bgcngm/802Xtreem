#!/usr/bin/perl -w


use strict;
use Tie::File;


die "usage: $0 <kernel_version> install|uninstall\n" if $#ARGV + 1 < 2;

my ($kernel_version, $action) = @ARGV;
my @file;
my $drv_name = 'sequans_usb.ko';
my $drv_path = 'updates/dkms';
my $file_name = "/lib/modules/".$kernel_version."/modules.order";


tie(@file, 'Tie::File', $file_name) or die "Can't open file $file_name: $!\n";


sub install()
{
	for (my $i = 0; $i < @file; $i++) {
	    if ($file[$i] =~ /cdc_ether/) {
		splice(@file, $i, 0, "$drv_path/$drv_name");
		last;
	    }
	}
}

sub uninstall()
{
	for (my $i = 0; $i < @file; $i++) {
	    if ($file[$i] =~ /$drv_name/) {
		splice(@file, $i, 1);
		last;
	    }
	}
}

if ('install' eq $action) {
	print "Add sequans_usb to kernel $kernel_version driver's list\n";
	install();
} elsif ('uninstall' eq $action) {
	print "Remove sequans_usb from kernel $kernel_version driver's list\n";
	uninstall();
} else {
	print "Unknown action '$action', supported actions are: install, uninstall\n";
	exit 1;
}

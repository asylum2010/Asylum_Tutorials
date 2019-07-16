
int main()
{
	int i = 2;
	int j = 4;
	int k;

	k = 3 + (i * 2) + 8 + 3 - 5 * j - (10 * j);

	print "\nThe value of k is: ";
	print k;
	print "\nand it is: ";

	if (k < 0) {
		print "negative\n";
	} else {
		print "positive\n";
	}

	return 0;
}

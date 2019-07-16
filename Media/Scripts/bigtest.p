
void print_positive()
{
	int i = 0;
	
	while (i < 10) {
		print i;
		print " ";
		
		++i;
	}
	
	print "\n";
}

void print_negative()
{
	int i = 0;
	
	while (i > -10) {
		print i;
		print " ";
		
		--i;
	}
	
	print "\n";
}

int max(int a, int b)
{
	if (a < b) {
		return b;
	}
	
	return a;
}

int main()
{
	print_positive();
	print_negative();

	int i = max(6, 2);
	int j, k;

	j = k = i;
	k = (k < 3);

	print "\nmax(6, 2) is: ";
	print i;
	print "\nj == ";
	print j;
	print "\nk == ";
	print k;
	
	print "\n(i > 3) is: ";
	print (i > 3);
	print "\n";
	
	if (!(i > 6 || i < 6)) {
		print "i is 6\n";
	}
	
	if (j == 6 && k != 1) {
		print "I suppose the program runs correctly.\n";
	} else {
		print "I suppose the program does not run correctly.\n";
	}
	
	return 0;
}

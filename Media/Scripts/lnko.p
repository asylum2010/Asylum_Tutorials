
int lnko(int a, int b)
{
	int c;
	
	while (b != 0) {
		c = b;
		b = a % b;
		a = c;
	}
	
	return a;
}

int main()
{
	int a = 16;
	int b = 28;
	
	print "The greatest common divisor of ";
	print a;
	print " and ";
	print b;
	print " is: ";
	print lnko(a, b);
	print "\n";
	
	return 0;
}

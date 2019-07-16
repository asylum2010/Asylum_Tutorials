
int factorial(int i)
{
	if (i == 0) {
		return 1;
	}
	
	return i * factorial(i - 1);
}

int main()
{
	int i = factorial(5);
	
	print "The factorial of 5 is: ";
	print i;
	print "\n";
		
	return 0;
}

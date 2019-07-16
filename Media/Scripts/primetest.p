
int is_prime(int x)
{
	int i = 2;

	while (i * i <= x) {
		if (x % i == 0) {
			return 0;
		}

		++i;
	}

	return 1;
}

int main()
{
	int count = 20;
	int i = 2;

	print "Prime numbers under ";
	print count;
	print " are:";

	while (i < count) {
		if (is_prime(i)) {
			print " ";
			print i;
		}

		++i;
	}

	print "\n";

	return 0;
}

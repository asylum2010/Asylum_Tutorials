
int main()
{
	int i = 6;
	int j = 5;
	
	if (i > 0) {
		int i = 7;
		int j = 6;
		
		if (i == 7) {
			print "Second scope is alive (i == ";
		} else {
			print "First scope is alive (i == ";
		}
		
		print i;
		print ")\n";
	}
	
	print "(j == ";
	print j;
	print ")\n";
	
	return 0;
}

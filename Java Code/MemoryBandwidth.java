class MemoryBandwidth {
	private static final int MINBYTES = (1 << 18);
	private static final int MAXBYTES = (1 << 27);
	private static final int MAXSTRIDE = 64;
	private static final int MAXELEMS = MAXBYTES * 8 / Integer.SIZE;
	private static int data[] = new int[MAXELEMS];
	private static volatile int sink;
	private static final int CORES = 4;
	private static SumThread[] st;

	public static void main(String[] args) {
		Compiler.disable();
		initData();
		System.out.print("Memory Bandwidth (MB/sec)\n");
		System.out.print("Size");
		System.out.print("\t");
		for (int stride = 1; stride <= MAXSTRIDE; stride = stride * 2)
			System.out.print("s" + stride + "\t");
		System.out.print("\n");
		for (int size = MAXBYTES; size >= MINBYTES; size >>= 1) {
			if (size > (1 << 20))
				System.out.print(size / (1 << 20) + "m\t");
			else
				System.out.print(size / 1024 + "k\t");

			for (int stride = 1; stride <= MAXSTRIDE; stride = stride * 2) {
				System.out.print(run(size, stride) + "\t");
			}
			System.out.println();
		}
	}

	private static void initData() {

		for (int i = 0; i < MAXELEMS; i++)
			data[i] = i;
	}

	private static void testThread(int elems, int stride) {
		for (int i = 0; i < CORES; i++) {
			st[i].start();
		}
		/* wait for the threads to finish! */
		try {
			for (int i = 0; i < CORES; i++) {
				st[i].join();
			}
		} catch (InterruptedException e) {
			System.out.println("Interrupted");
		}
	}

	private static void prepareThread(int elems, int stride) {
		st = new SumThread[CORES];
		for (int i = 0; i < CORES; i++) {
			st[i] = new SumThread(i * (elems / CORES),
					(i * (elems / CORES) + (elems / CORES)), data);
		}
	}

	private static void test(int elems, int stride) {
		int i;
		int result = 0;
		sink = 0;
		for (i = 0; i < elems; i += stride) {
			result += data[i];
		}
		sink = result;
	}

	private static double run(int size, int stride) {
		int elems = size * 8 / Integer.SIZE;
		prepareThread(elems, stride);
		testThread(elems, stride);
		prepareThread(elems, stride);
		// test(elems, stride);
		long startTime = System.nanoTime();
		testThread(elems, stride);
		// test(elems, stride);
		long endTime = System.nanoTime();
		return (size / (stride)) / ((endTime - startTime) / 1000);
	}
}

class SumThread extends Thread {
	public int from, to, sum;
	public int data[];

	public SumThread() {

	}

	public SumThread(int from, int to, int[] data) {
		this.from = from;
		this.to = to;
		this.data = new int[data.length];
		for (int i = 0; i < data.length; i++) {
			this.data[i] = data[i];
		}
		sum = 0;
	}

	public void run() {
		for (int i = from; i <= to; i++) {
			sum += i;
		}
	}

	public int getSum() {
		return sum;
	}
}
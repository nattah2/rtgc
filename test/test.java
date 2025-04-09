import java.util.ArrayList;

public class GCBenchmark {
    static final int TEST_DURATION_MS = 10000;
    static final int ALLOCATION_SIZE = 1024 * 100; // 100KB
    static final int ALLOCATION_INTERVAL_MS = 10;
    static final int HIGH_PRIORITY_INTERVAL_MS = 50;

    static volatile boolean running = true;

    public static void main(String[] args) throws InterruptedException {
        // Start high-priority task
        Thread highPriorityThread = new Thread(() -> {
            long start = System.currentTimeMillis();
            long next = start + HIGH_PRIORITY_INTERVAL_MS;
            ArrayList<Long> latencies = new ArrayList<>();

            while (running) {
                long now = System.currentTimeMillis();
                long latency = now - next + HIGH_PRIORITY_INTERVAL_MS;
                latencies.add(latency);
                System.out.println("[HighPriority] Latency: " + latency + " ms");

                // Simulate work
                doWork(1_000_000);

                try {
                    Thread.sleep(Math.max(0, next - System.currentTimeMillis()));
                } catch (InterruptedException ignored) {}
                next += HIGH_PRIORITY_INTERVAL_MS;
            }

            System.out.println("High Priority Task Latencies (ms): " + latencies);
        });

        // Start low-priority GC-heavy task
        Thread lowPriorityThread = new Thread(() -> {
            ArrayList<byte[]> allocations = new ArrayList<>();
            while (running) {
                allocations.add(new byte[ALLOCATION_SIZE]);
                try {
                    Thread.sleep(ALLOCATION_INTERVAL_MS);
                } catch (InterruptedException ignored) {}
            }
        });

        highPriorityThread.setPriority(Thread.MAX_PRIORITY);
        lowPriorityThread.setPriority(Thread.MIN_PRIORITY);

        highPriorityThread.start();
        lowPriorityThread.start();

        Thread.sleep(TEST_DURATION_MS);
        running = false;

        highPriorityThread.join();
        lowPriorityThread.join();
    }

    static void doWork(int iterations) {
        double dummy = 0;
        for (int i = 0; i < iterations; i++) {
            dummy += Math.sin(i);
        }
    }
}

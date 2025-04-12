import java.util.ArrayList;
import java.util.List;
import java.io.FileWriter;
import java.io.IOException;
import java.lang.management.GarbageCollectorMXBean;
import java.lang.management.ManagementFactory;

public class GCTest {
    private static final int MAX_ITERATIONS = 100_000;
    private static final int CHILDREN_PER_NODE = 10;
    private static final int CLEAR_THRESHOLD = 10_000;
    private static final String CSV_FILE = "gc_test_results.csv";

    public static void main(String[] args) {
        logJVMConfig(); // Log JVM/GC settings
        List<Node> roots = new ArrayList<>();
        List<String> csvData = new ArrayList<>();
        csvData.add("Iteration,TaskDelay(us),MemoryUsage(MB),GCCount,TotalGCTime(ms)");

        List<GarbageCollectorMXBean> gcBeans = ManagementFactory.getGarbageCollectorMXBeans();
        long initialGcCount = getTotalGCCount(gcBeans);
        long initialGcTime = getTotalGCTime(gcBeans);

        for (int i = 0; i < MAX_ITERATIONS; i++) {
            Node root = new Node(CHILDREN_PER_NODE);
            roots.add(root);
            if (roots.size() > CLEAR_THRESHOLD) roots.clear();

            long start = System.nanoTime();
            try {
                Thread.sleep(1);
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
                System.err.println("Interrupted: " + e.getMessage());
                break;
            }
            long taskDelayUs = (System.nanoTime() - start) / 1000;

            // Capture metrics
            long memoryUsageMB = (Runtime.getRuntime().totalMemory() - Runtime.getRuntime().freeMemory()) / (1024 * 1024);
            long currentGcCount = getTotalGCCount(gcBeans) - initialGcCount;
            long currentGcTime = getTotalGCTime(gcBeans) - initialGcTime;

            csvData.add(String.format("%d,%d,%d,%d,%d",
                i, taskDelayUs, memoryUsageMB, currentGcCount, currentGcTime));

            // Log GC events if they occurred
            if (currentGcCount > (i == 0 ? 0 : csvData.size() - 2)) {
                System.out.printf("[GC] Iteration %d: %d collections, %d ms total\n",
                    i, currentGcCount, currentGcTime);
            }
        }

        writeCSV(csvData);
    }

    private static long getTotalGCCount(List<GarbageCollectorMXBean> gcBeans) {
        return gcBeans.stream().mapToLong(GarbageCollectorMXBean::getCollectionCount).sum();
    }

    private static long getTotalGCTime(List<GarbageCollectorMXBean> gcBeans) {
        return gcBeans.stream().mapToLong(GarbageCollectorMXBean::getCollectionTime).sum();
    }

    private static void logJVMConfig() {
        System.out.println("[JVM] GC: " + ManagementFactory.getGarbageCollectorMXBeans()
            .stream().map(GarbageCollectorMXBean::getName).reduce((a, b) -> a + ", " + b).orElse("Unknown"));
        System.out.println("[JVM] Max Heap: " + Runtime.getRuntime().maxMemory() / (1024 * 1024) + " MB");
    }

    private static void writeCSV(List<String> csvData) {
        try (FileWriter writer = new FileWriter(CSV_FILE)) {
            for (String line : csvData) writer.write(line + "\n");
            System.out.println("[JVM] Results saved to: " + CSV_FILE);
        } catch (IOException e) {
            System.err.println("Failed to write CSV: " + e.getMessage());
        }
    }
}

class Node {
    Node[] children;
    Node(int n) { children = new Node[n]; }
}

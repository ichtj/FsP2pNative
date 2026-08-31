package com.p2p.sample;

import android.os.Bundle;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;

import com.library.natives.BaseData;
import com.library.natives.BaseFsP2pTools;
import com.library.natives.IPipelineCallback;
import com.library.natives.Infomation;
import com.library.natives.PutType;
import com.library.natives.Type;
import com.library.natives.XCoreBean;

import org.junit.After;
import org.junit.Assume;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.io.File;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

@RunWith(AndroidJUnit4.class)
public class PipelineLifecycleStressTest {
    private static final String CLIENT_SN = "lifecycle-stress";
    private static final String PRODUCT_ID = "stress-product";

    @After
    public void tearDown() {
        BaseFsP2pTools.disConnect();
    }

    @Test
    public void repeatedConnectDuringBroadcastTrafficDoesNotLeakPipelineWorkers()
            throws Exception {
        Bundle arguments = InstrumentationRegistry.getArguments();
        Assume.assumeTrue("Set lifecycleStress=true and provide an MQTT broker on port 1883",
                "true".equals(arguments.getString("lifecycleStress")));

        BaseFsP2pTools.logEnable(true);
        BaseFsP2pTools.disConnect();
        int baselineThreads = currentThreadCount();

        Infomation information = new Infomation(
                CLIENT_SN, PRODUCT_ID, "stress", "stress", Type.Unknown, 1);
        XCoreBean broker = new XCoreBean("127.0.0.1", 1883, "", "");
        StressCallback callback = new StressCallback();

        callback.prepareForConnection();
        BaseFsP2pTools.connect(information, broker, "{}", callback);
        assertTrue("The test MQTT broker was not reached",
                callback.awaitConnection());

        AtomicBoolean publishing = new AtomicBoolean(true);
        AtomicInteger successfulPublishes = new AtomicInteger();
        Thread publisher = new Thread(() -> {
            int sequence = 0;
            while (publishing.get()) {
                Map<String, Object> values = new HashMap<>();
                values.put("sequence", sequence++);
                if (BaseFsP2pTools.postMsg(
                        PutType.BROADCAST,
                        CLIENT_SN,
                        PRODUCT_ID,
                        "Heartbeat",
                        values)) {
                    successfulPublishes.incrementAndGet();
                }
            }
        }, "fsp2p-broadcast-stress");
        publisher.start();

        ExecutorService callers = Executors.newFixedThreadPool(4);
        try {
            for (int caller = 0; caller < 4; caller++) {
                callers.execute(() -> {
                    for (int iteration = 0; iteration < 100; iteration++) {
                        BaseFsP2pTools.connect(information, broker, "{}", callback);
                    }
                });
            }
            callers.shutdown();
            assertTrue("Concurrent connect calls did not finish",
                    callers.awaitTermination(30, TimeUnit.SECONDS));

            long publishDeadline = System.nanoTime() + TimeUnit.SECONDS.toNanos(10);
            while (successfulPublishes.get() <= 10 &&
                   System.nanoTime() < publishDeadline) {
                Thread.sleep(10);
            }
            assertTrue("No broadcast traffic was generated",
                    successfulPublishes.get() > 10);

            for (int cycle = 0; cycle < 20; cycle++) {
                BaseFsP2pTools.disConnect();
                assertFalse("Pipeline still reports connected after disConnect",
                        BaseFsP2pTools.getConnectStatus());

                callback.prepareForConnection();
                BaseFsP2pTools.connect(information, broker, "{}", callback);
                assertTrue("Reconnect did not complete in cycle " + cycle,
                        callback.awaitConnection());
            }
        } finally {
            callers.shutdownNow();
            publishing.set(false);
            publisher.join(10_000);
        }

        assertFalse("Broadcast publisher did not stop", publisher.isAlive());

        BaseFsP2pTools.disConnect();
        Thread.sleep(500);
        assertFalse("Pipeline still reports connected after disConnect",
                BaseFsP2pTools.getConnectStatus());
        assertTrue("Native pipeline worker threads did not return to baseline",
                currentThreadCount() <= baselineThreads + 4);
    }

    private static int currentThreadCount() {
        String[] tasks = new File("/proc/self/task").list();
        return tasks == null ? Integer.MAX_VALUE : tasks.length;
    }

    private static final class StressCallback implements IPipelineCallback {
        private final Semaphore connections = new Semaphore(0);

        void prepareForConnection() {
            connections.drainPermits();
        }

        boolean awaitConnection() throws InterruptedException {
            return connections.tryAcquire(10, TimeUnit.SECONDS);
        }

        @Override
        public void p2pConnState(boolean isConnected, String description) {
            if (isConnected) connections.release();
        }

        @Override
        public void iotConnState(boolean connected, String description) {}

        @Override
        public void msgArrives(BaseData baseData) {}

        @Override
        public void pushed(BaseData data) {}

        @Override
        public void iotReplyed(String act, String iid) {}

        @Override
        public void pushFail(BaseData baseData, String description) {}

        @Override
        public void subscribed(String topic) {}

        @Override
        public void subscribeFail(String topic, String description) {}
    }
}

/*
 * ALMA - Atacama Large Millimiter Array (c) European Southern Observatory, 2006
 * 
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2.1 of the License, or (at your option)
 * any later version.
 * 
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 * 
 */
package alma.lasersource.test;

import java.sql.Timestamp;
import java.util.Collection;
import java.util.Random;
import java.util.Vector;

import cern.laser.source.alarmsysteminterface.FaultState;
import cern.laser.source.alarmsysteminterface.impl.ASIMessageHelper;
import cern.laser.source.alarmsysteminterface.impl.XMLMessageHelper;
import cern.laser.source.alarmsysteminterface.impl.message.ASIMessage;

import com.cosylab.acs.jms.ACSJMSMessageEntity;

import alma.acs.component.client.ComponentClientTestCase;
import alma.acs.nc.Consumer;
import alma.alarmsystem.source.ACSAlarmSystemInterface;
import alma.alarmsystem.source.ACSAlarmSystemInterfaceFactory;
import alma.alarmsystem.source.ACSFaultState;

/**
 * A stress test: it sends a great number of alarm sources and checkes
 *  - if all of them have been published and if their
 *  - they arrived in the right order
 *  - thier contents match with the sent fault states
 *   
 * @author acaproni
 *
 */
public class SourceStressTest extends ComponentClientTestCase {
	
	/**
	 * The relevant fields of a fault state to compare against
	 * the fault states received from the NC
	 * 
	 * @author acaproni
	 *
	 */
	private class MiniFaultState {
		// The fields of the fault state
		public final String FF, FM;
		public final int FC;
		public final long msec;
		public final String description; // ACTIVE/Terminate
		public final Timestamp timestamp;
		
		public MiniFaultState() {
			FF=SourceStressTest.FF+Math.abs(rnd.nextInt());
			FM=SourceStressTest.FM+Math.abs(rnd.nextInt());
			FC=Math.abs(rnd.nextInt());
			msec=System.currentTimeMillis();
			timestamp=new Timestamp(msec);
			if (rnd.nextInt()%2==0) {
				description=FaultState.ACTIVE;
			} else {
				description=FaultState.TERMINATE;
			}
			assertNotNull(FF);
			assertNotNull(FM);
			assertNotNull(description);
			assertNotNull(timestamp);
		}
	}
	
	// The consumer
	private Consumer m_consumer;

	// The number of fault states to send with the same source
	private static final int NUM_OF_FS_TO_SEND_ONE_SOURCE = 10000;
	
	// The number of fault states to send with the more sources
	// Chenging this number remeber that CERN code SynchroBuffer
	// create a thread per each source
	private static final int NUM_OF_FS_TO_SEND_MORE_SOURCES = 1000;
	
	// The NC name to listen for published fault states
	private static final String m_channelName = "CMW.ALARM_SYSTEM.ALARMS.SOURCES.ALARM_SYSTEM_SOURCES";
	
	// The fault states have a random FF and a random FM each of which
	// is generated by appending a random number to the following strings 
	private static String FF = "Family";
	private static String FM = "Member";
	
	// The random number generator to create FF and FM
	private static Random rnd = new Random(System.currentTimeMillis());
	
	// The source
	private ACSAlarmSystemInterface alarmSource;
	
	// The fault states received from the NC
	private Vector<FaultState> receivedFS;
	
	private MiniFaultState[] statesToPublish;
	
	/**
	 * Constructor 
	 * @throws Exception
	 */
	public SourceStressTest() throws Exception {
		super("Source stress test");
	}
	
	/**
	 * 
	 * @see alma.acs.component.client.ComponentClientTestCase#tearDown()
	 */
	protected void setUp() throws Exception {
		// TODO Auto-generated method stub
		super.setUp();
		assertNotNull(getContainerServices());
		
		ACSAlarmSystemInterfaceFactory.init(getContainerServices());
		
		
		m_consumer= new Consumer(m_channelName, alma.acsnc.ALARMSYSTEM_DOMAIN_NAME.value, getContainerServices());
		assertNotNull("Error instantiating the consumer",m_consumer);
		m_consumer.addSubscription(com.cosylab.acs.jms.ACSJMSMessageEntity.class, this);
		m_consumer.consumerReady();
		
		alarmSource = ACSAlarmSystemInterfaceFactory.createSource();
		assertNotNull("Error instantiating the source",alarmSource);
		
		receivedFS= new Vector<FaultState>();
		assertNotNull(receivedFS);
	}

	/**
	 * 
	 * @see alma.acs.component.client.ComponentClientTestCase#tearDown()
	 */
	protected void tearDown() throws Exception {
		m_consumer.disconnect();
		super.tearDown();
	}
	
	public void receive(ACSJMSMessageEntity msg) throws Exception {
		ASIMessage asiMsg = XMLMessageHelper.unmarshal(msg.text);
		Collection<FaultState>faultStates = ASIMessageHelper.unmarshal(asiMsg);
		assertNotNull(faultStates);
		for (FaultState fs: faultStates) {
			assertNotNull(fs);
			synchronized (receivedFS) {
				receivedFS.add(fs);	
			}
		}
	}
	
	/**
	 * Send a fault state to the NC.
	 * It uses the global source or build a new one depending on the
	 * parameter
	 * 
	 * @param mfs The fault state to publish
	 * @param sameSource If true the same source is used to send the alarm
	 *                   if true a new source is built and the fault state is
	 *                           sent using this new source
	 */
	private void send(MiniFaultState mfs, boolean sameSource) throws Exception {
		assertNotNull(mfs);
		ACSFaultState fs = ACSAlarmSystemInterfaceFactory.createFaultState(mfs.FF, mfs.FM, mfs.FC);
		assertNotNull("Error instantiating the FS",fs);
		fs.setDescriptor(mfs.description);
		fs.setUserTimestamp(mfs.timestamp);
		if (sameSource) {
			alarmSource.push(fs);
		} else {
			ACSAlarmSystemInterface newSource= ACSAlarmSystemInterfaceFactory.createSource();
			assertNotNull(newSource);
			newSource.push(fs);
		}
	}
	
	private void checkFaultStates(MiniFaultState sent, FaultState recv) throws Exception {
		assertEquals(sent.FF, recv.getFamily());
		assertEquals(sent.FM, recv.getMember());
		assertEquals(sent.FC, recv.getCode());
		assertEquals(sent.description, recv.getDescriptor());
		assertEquals(sent.timestamp, recv.getUserTimestamp());
	}
	
	/**
	 * Build the data to publish and check for correctness
	 * 
	 * @param len The number of items to biuld and put in the array
	 */
	private void buildData(int len) throws Exception {
		statesToPublish= new MiniFaultState[len];
		assertNotNull(statesToPublish);
		// Build the array of state to publish
		for (int t=0; t<statesToPublish.length; t++) {
			MiniFaultState fs = new MiniFaultState();
			assertNotNull(fs);
			statesToPublish[t]=fs;
		}
	}
	
	/**
	 * Wait until all the fault states are received or a timeout happened.
	 * If all the fault states are received, it compares what has been sent
	 * with what has been received.
	 */
	private void waitAndCheck() throws Exception {
		int timeout = 60; // timeout in secs
		int count=0;
		int old=0; // The number of items read in the previous iteration
		// Wait for all the alarms to be in the vector
		while (receivedFS.size()<statesToPublish.length && count<2*timeout) {
			if (old!=receivedFS.size()) {
				count=0;
				old=receivedFS.size();
			}
			try {
				Thread.sleep(500);
				count++;
			} catch (Exception e) {}
		}
		assertEquals(statesToPublish.length, receivedFS.size());
		
		for (int t=0; t< statesToPublish.length; t++) {
			checkFaultStates(statesToPublish[t], receivedFS.get(t));
		}
	}
	
	/**
	 * Test by sending all the fault states using the same source. 
	 * When all the alarms have arrived it checks for their
	 * correctness 
	 * 
	 * @throws Exception
	 */
	public void testStressSameSource() throws Exception {
		buildData(NUM_OF_FS_TO_SEND_ONE_SOURCE);
		
		// Send the alarms
		for (int t=0; t<statesToPublish.length; t++) {
			send(statesToPublish[t],true);
		}
		
		waitAndCheck();
	}
	
	/**
	 * Test by sending all the fault states using the same source. 
	 * When all the alarms have arrived it checks for their
	 * correctness 
	 * 
	 * @throws Exception
	 */
	public void testStressDifferentSources() throws Exception {
		buildData(NUM_OF_FS_TO_SEND_MORE_SOURCES);
		
		// Send the alarms
		for (int t=0; t<statesToPublish.length; t++) {
			send(statesToPublish[t],false);
		}
		
		waitAndCheck();
	}
}

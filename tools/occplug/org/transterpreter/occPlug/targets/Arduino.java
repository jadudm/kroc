/**
 * 
 */
package org.transterpreter.occPlug.targets;

/*
 * Arduino.java
 * part of the occPlug plugin for the jEdit text editor
 * Copyright (C) 2009 Christian L. Jacobsen
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

import java.awt.Component;
import java.awt.event.FocusEvent;
import java.awt.event.FocusListener;
import java.io.File;
import java.io.FilenameFilter;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Iterator;
import java.util.SortedSet;
import java.util.TreeSet;

import javax.swing.DefaultComboBoxModel;
import javax.swing.JComboBox;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.event.PopupMenuEvent;
import javax.swing.event.PopupMenuListener;

import org.gjt.sp.jedit.Buffer;
import org.gjt.sp.jedit.MiscUtilities;
import org.gjt.sp.jedit.jEdit;
import org.transterpreter.occPlug.OccPlugPlugin;
import org.transterpreter.occPlug.OccPlugUtil;
import org.transterpreter.occPlug.OccPlug.DocumentWriter;
import org.transterpreter.occPlug.process.ExecWorker;
import org.transterpreter.occPlug.targets.support.BaseTarget;
import org.transterpreter.occPlug.targets.support.CompileAbility;
import org.transterpreter.occPlug.targets.support.CompileTarget;
import org.transterpreter.occPlug.targets.support.FirmwareAbility;
import org.transterpreter.occPlug.targets.support.FirmwareTarget;
import org.transterpreter.occPlug.targets.support.OccbuildHelper;
import org.transterpreter.occPlug.targets.support.OccbuildOptions;
import org.transterpreter.occPlug.targets.support.OccbuildTVMOptions;
import org.transterpreter.occPlug.targets.support.TargetExecWorkerHelper;

/**
 * @author clj
 * 
 */
public class Arduino extends BaseTarget implements FirmwareAbility,
		CompileAbility {

	private final FirmwareTarget[]		firmwareTargets			= { 
			new FirmwareTarget("Arduino (and compatible)", this) };
	private final CompileTarget[]		compileTargets			= { 
			new CompileTarget("Arduino (and compatible)", this) };
	private final DefaultComboBoxModel	arduinoPort				= new DefaultComboBoxModel();
	private final SortedSet				arduinoPortItems		= new TreeSet();
	private final JPanel				arduinoFirmwareOptions;
	private final JPanel				arduinoCompileOptions;
	private final ArrayList				disableOnDownload		= new ArrayList();

	public Arduino()
	{
		super();
		
		arduinoFirmwareOptions = makeOptionsPanel();
		arduinoCompileOptions = makeOptionsPanel();
	}
	
	public FirmwareTarget[] getFirmwareTargets() {
		return firmwareTargets;
	}

	protected JPanel makeOptionsPanel()
	{
		/* Focus Listener for option saving */
		final FocusListener saveFirmwareOptionsFocusListener = new FocusListener() {
			public void focusGained(FocusEvent e) {
				// Not used
			}

			public void focusLost(FocusEvent e) {
				saveOptions();
			}
		};

		/* Arduino options */
		JPanel options = new JPanel();
		options.add(new JLabel("Port: "));
		JComboBox port = new JComboBox(arduinoPort);
		port.setEditable(true);
		PopupMenuListener portPopupListener = new PopupMenuListener() {
			public void popupMenuCanceled(PopupMenuEvent e) {
				// Not used
			}

			public void popupMenuWillBecomeInvisible(PopupMenuEvent e) {
				// Not used
			}

			public void popupMenuWillBecomeVisible(PopupMenuEvent e) {
				FilenameFilter filter = new FilenameFilter() {
					public boolean accept(File dir, String name) {
						return (name.startsWith("tty.usbserial-")
								|| name.equals("ttys0") || name.equals("ttys1")
								|| name.equals("ttys2") || name.equals("ttys3"));
					}
				};
				File dir = new File("/dev");
				String[] devices = dir.list(filter);

				for (int i = 0; i < devices.length; i++) {
					if (arduinoPortItems.add("/dev/" + devices[i])) {
						arduinoPort.addElement("/dev/" + devices[i]);
					}
				}
			}
		};
		portPopupListener.popupMenuWillBecomeVisible(null);
		port.addFocusListener(saveFirmwareOptionsFocusListener);
		port.getEditor().getEditorComponent().addFocusListener(
				saveFirmwareOptionsFocusListener);
		port.addPopupMenuListener(portPopupListener);
		port.setSelectedItem(jEdit
				.getProperty(OccPlugPlugin.OPTION_PREFIX
						+ "targets.arduino.port"));
		disableOnDownload.add(port);
		options.add(port);

		return options;		
	}
	
	public JPanel getFirmwareOptions(FirmwareTarget target) {
		return arduinoFirmwareOptions;
	}
	
	public JPanel getCompileOptions(CompileTarget target) {
		return arduinoCompileOptions;
	}
	
	protected void saveOptions() {
		jEdit.setProperty(OccPlugPlugin.OPTION_PREFIX
				+ "targets.arduino.port", (String) arduinoPort
				.getSelectedItem());
	}

	public void uploadFirmware(FirmwareTarget target, DocumentWriter output,
			Runnable finished) {
		String port = (String) arduinoPort.getSelectedItem();
		if (port == null || port.trim().equals("")) {
			output.writeError("Please specify a port");
			finished.run();
			return;
		}

		final String[] firmdlCommand = { 
				OccPlugUtil.pathifyXXX("bin/avrdude"),
				"-C", OccPlugUtil.pathifyXXX("lib/avrdude.conf"), 
				"-U", "flash:w:" + OccPlugUtil.pathifyXXX("share/tvm-arduino/firmware/tvm-arduino.hex"),
				"-F", 
				"-P", (String) arduinoPort.getSelectedItem(),
				// FIXME: Need a sensible way of setting these
				("-c"), "stk500v1", 
				"-p", "atmega328p", 
				"-b", "57600", };

		output.writeRegular("Downloading Plumbing firmware\n");
		OccPlugUtil.writeVerbose("Command: " + Arrays.asList(firmdlCommand) + " \n", output);

		final Runnable[] finalisers = { finished };
		ExecWorker execWorker = new ExecWorker(firmdlCommand, null, null, // new
																			// File(workingDir),
				new TargetExecWorkerHelper("firmware download", output,
						finalisers));

		execWorker.start();
	}

	public void setEnabledForFirmwareOptions(boolean enabled) {
		Iterator i = disableOnDownload.iterator();
		while (i.hasNext()) {
			Component item = (Component) i.next();
			item.setEnabled(enabled);
		}
	}

	public void compileProgram(CompileTarget target, Buffer buffer,
			DocumentWriter output, Runnable finished) {
		final String occFile = buffer.getName();
		
		if (!occFile.toLowerCase().endsWith(".occ")) {
			output.writeError("Error: Only occam (.occ) source files can be compiled.\n");
			output.writeError("       The current buffer does not contain a .occ file!\n");
			finished.run();
			return;
		}
	
		OccbuildOptions options = new OccbuildTVMOptions();
		//options.target_cpu = "avr";
		options.occbuildName = "avr-occbuild";
		options.systemSearch = new String[] {
				OccPlugUtil.pathifyXXX("share/tvm-arduino/plumbing-include"),
				OccPlugUtil.pathifyXXX("share/tvm-arduino/avr-vtlib"),
				OccPlugUtil.pathifyXXX("share/tvm-arduino/avr-vtinclude")};
		// FIXME: Needs to be settable somewhere
		options.defines.put("F.CPU", "16000000"); 
		String[] occbuildCommand = OccbuildHelper.makeOccbuildProgramCommand(options, occFile);
		
		// Say what we are doing
		output.writeRegular("Compiling: " + occFile + "\n");
		OccPlugUtil.writeVerbose("Command: " + Arrays.asList(occbuildCommand) + "\n", output);

		final String[] env = OccbuildHelper.makeOccbuildEnvironment();
		OccPlugUtil.writeVerbose("Environment: " + Arrays.asList(env) + "\n", output);
		
		final Runnable[] finalisers = { finished };
		ExecWorker execWorker = new ExecWorker(occbuildCommand, 
				env, 
				new File(buffer.getDirectory()),
				new TargetExecWorkerHelper("compile", output,
						finalisers));

		execWorker.start();		
		
	}


	public CompileTarget[] getCompileTargets() {
		return compileTargets;
	}

	public void runProgram(final CompileTarget theTarget, final Buffer buffer,
			final DocumentWriter output, final Runnable finished) {
		
		String port = (String) arduinoPort.getSelectedItem();
		if (port == null || port.trim().equals("")) {
			output.writeError("Please specify a port");
			finished.run();
			return;
		}
		
		final String fileBase = MiscUtilities.getFileNameNoExtension(buffer.getName());
		final String tbcFile = fileBase + ".tbc";
		final String ihexFile = fileBase + ".ihex";
		final File curDir = new File(buffer.getDirectory());
		final String [] ihexCommand = {
				OccPlugUtil.pathifyXXX("bin/binary-to-ihex"),
				// FIXME: This needs to come from elsewhere
				"0x5000",
				tbcFile,
				ihexFile
		};

		final String[] runCommand = { 
				OccPlugUtil.pathifyXXX("bin/avrdude"),
				"-C", OccPlugUtil.pathifyXXX("lib/avrdude.conf"), 
				"-V", 
				"-F", 
				"-P", (String) arduinoPort.getSelectedItem(),
				"-D",
				// FIXME: Need a sensible way of setting these
				("-c"), "stk500v1", 
				"-p", "atmega328p", 
				"-b", "57600", 
				"-U", "flash:w:" + ihexFile};
		final Runnable[] finalisers = { finished };
		
		final Runnable[] intermediaryFinalisers =
		{
				new Runnable() {
					public void run() {
						ExecWorker execWorker = new ExecWorker(runCommand, 
								null, 
								curDir,
								new TargetExecWorkerHelper("run", output,
										finalisers));
						execWorker.start();
					}
				}
		};
		ExecWorker execWorker = new ExecWorker(ihexCommand, 
				null, 
				curDir,
				new TargetExecWorkerHelper("run", output,
						intermediaryFinalisers));
		execWorker.start();
	}
}

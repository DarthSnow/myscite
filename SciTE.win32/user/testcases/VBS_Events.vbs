
Dim oIE, bExit
wscript.Quit(Main)

  Function Main()
     ' ---- Create object and connect the event handler in one step.
    Set oIE = wscript.CreateObject("InternetExplorer.Application","IE_")

        bExit = false
    oIE.navigate ""  ' Load URL
    oIE.Visible = 1   ' Keep visible.

    msgbox("Waiting for IE to quit")
    do :  wscript.sleep(300) : loop until bExit=true
    
    Main=0
  End Function 
 
  Sub IE_onQuit()
    msgbox("IE_onQuit Recieved")
    bExit=true
  End Sub

' original Sample
'https://technet.microsoft.com/de-de/ie/aa366443



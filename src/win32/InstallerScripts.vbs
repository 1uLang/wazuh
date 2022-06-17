
' Script for configuration Hids agent.
' Copyright (C) 2015-2020, Hids Inc.
'
' This program is free software; you can redistribute it and/or modify
' it under the terms of the GNU General Public License as published by
' the Free Software Foundation; either version 3 of the License, or
' (at your option) any later version.
'
' This program is distributed in the hope that it will be useful,
' but WITHOUT ANY WARRANTY; without even the implied warranty of
' MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
' GNU General Public License for more details.
'
' You should have received a copy of the GNU General Public License
' along with this program; if not, write to the Free Software Foundation,
' Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
'
' ------------------------------------------------'

On Error Resume Next

public function config()

' Custom parameters
strArgs = Session.Property("CustomActionData")
args = Split(strArgs, ",")
home_dir        = Replace(args(0), Chr(34), "")
address         = Replace(args(1), Chr(34), "")
server_port     = Replace(args(2), Chr(34), "")
protocol        = Replace(args(3), Chr(34), "")
notify_time     = Replace(args(4), Chr(34), "")
time_reconnect  = Replace(args(5), Chr(34), "")

wazuh_address         = Replace(args(6), Chr(34), "")
wazuh_server_port     = Replace(args(7), Chr(34), "")
wazuh_protocol        = Replace(args(8), Chr(34), "")
wazuh_notify_time     = Replace(args(9), Chr(34), "")
wazuh_time_reconnect  = Replace(args(10), Chr(34), "")

If address = "" Then
    address = wazuh_address
End If

If server_port = "" Then
    server_port = wazuh_server_port
End If

If protocol = "" Then
    protocol = wazuh_protocol
End If

If notify_time = "" Then
    notify_time = wazuh_notify_time
End If

If time_reconnect = "" Then
    time_reconnect = wazuh_time_reconnect
End If

' Only try to set the configuration if variables are setted

Set objFSO = CreateObject("Scripting.FileSystemObject")

' Create an empty client.keys file on first install
If Not objFSO.fileExists(home_dir & "client.keys") Then
    objFSO.CreateTextFile(home_dir & "client.keys")
End If

If objFSO.fileExists(home_dir & "ossec.conf") Then
    ' Reading ossec.conf file
    Const ForReading = 1
    Set objFile = objFSO.OpenTextFile(home_dir & "ossec.conf", ForReading)

    strText = objFile.ReadAll
    objFile.Close

    If address <> "" or server_port <> "" or protocol <> "" or notify_time <> "" or time_reconnect <> "" Then
        If address <> "" and InStr(address,";") > 0 Then 'list of address
            ip_list=Split(address,";")
            formatted_list ="    </server>" & vbCrLf
            not_replaced = True
            for each ip in ip_list
                If not_replaced Then
                  strText = Replace(strText, "<address>0.0.0.0</address>", "<address>" & ip & "</address>")
                  not_replaced = False
                Else
                    formatted_list = formatted_list & "    <server>" & vbCrLf
                    formatted_list = formatted_list & "      <address>" & ip & "</address>" & vbCrLf
                    formatted_list = formatted_list & "      <port>1514</port>" & vbCrLf
                    formatted_list = formatted_list & "      <protocol>tcp</protocol>" & vbCrLf
                    formatted_list = formatted_list & "    </server>" & vbCrLf
                End If
            next
            strText = Replace(strText, "    </server>", formatted_list)
        ElseIf address <> "" and InStr(strText,"<address>") > 0 Then
            strText = Replace(strText, "<address>0.0.0.0</address>", "<address>" & address & "</address>")

        ElseIf address <> "" Then 'single address
            ' Fix for the legacy server-ip and server-hostname keynames
            Set re = new regexp
            re.Pattern = "<server-ip>.*</server-ip>"
            re.Global = True
            strText = re.Replace(strText, "<server-ip>" & address & "</server-ip>")
            re.Pattern = "<server-hostname>.*</server-hostname>"
            re.Global = True
            strText = re.Replace(strText, "<server-hostname>" & address & "</server-hostname>")
            strText = Replace(strText, "<address>0.0.0.0</address>", "<address>" & address & "</address>")
        End If

        If server_port <> "" Then ' manager server_port
            If InStr(strText, "<port>") > 0 Then
                strText = Replace(strText, "<port>1514</port>", "<port>" & server_port & "</port>")
            Else
                ' Fix for the legacy files (not including the key)
                strText = Replace(strText, "</client>", "  <port>" & server_port & "</port>"& vbCrLf &"  </client>")
            End If

        End If

        If protocol <> "" Then
            If InStr(strText, "<protocol>") > 0 Then
                Set re = new regexp
                re.Pattern = "<protocol>.*</protocol>"
                re.Global = True
                strText = re.Replace(strText, "<protocol>" & LCase(protocol) & "</protocol>")
            Else
            ' Fix for the legacy files (not including the key)
                strText = Replace(strText, "</client>", "   <protocol>" & LCase(protocol) & "</protocol>"& vbCrLf &"  </client>")
            End If
        End If

        If notify_time <> "" Then
            If InStr(strText, "<notify_time>") > 0 Then
                Set re = new regexp
                re.Pattern = "<notify_time>.*</notify_time>"
                re.Global = True
                strText = re.Replace(strText, "<notify_time>" & notify_time & "</notify_time>")
            Else
                ' Fix for the legacy files (not including the key)
                strText = Replace(strText, "</client>", "   <notify_time>" & notify_time & "</notify_time>"& vbCrLf &"  </client>")
            End If
        End If

        If time_reconnect <> "" Then 'TODO fix the - and use _
            If InStr(strText, "<time-reconnect>") > 0 Then
                Set re = new regexp
                re.Pattern = "<time-reconnect>.*</time-reconnect>"
                re.Global = True
                strText = re.Replace(strText, "<time-reconnect>" & time_reconnect & "</time-reconnect>")
            Else
                ' Fix for the legacy files (not including the key)
                strText = Replace(strText, "</client>", "   <time-reconnect>" & time_reconnect & "</time-reconnect>"& vbCrLf &"  </client>")

            End If
        End If

        ' Writing the ossec.conf file
        const ForWriting = 2
        Set objFile = objFSO.OpenTextFile(home_dir & "ossec.conf", ForWriting)
        objFile.WriteLine strText
        objFile.Close

    End If

	If Not objFSO.fileExists(home_dir & "local_internal_options.conf") Then

		If objFSO.fileExists(home_dir & "default-local_internal_options.conf") Then
			' Reading default-local_internal_options.conf file
			Set objFile = objFSO.OpenTextFile(home_dir & "default-local_internal_options.conf", ForReading)
			strText = objFile.ReadAll
			objFile.Close

			' Writing the local_internal_options.conf file
			Set objFile = objFSO.CreateTextFile(home_dir & "local_internal_options.conf", ForWriting)
			objFile.WriteLine strText
			objFile.Close
		Else
			Set objFile = objFSO.CreateTextFile(home_dir & "local_internal_options.conf", ForWriting)
			objFile.WriteLine("# local_internal_options.conf")
			objFile.WriteLine("#")
			objFile.WriteLine("# This file should be handled with care. It contains")
			objFile.WriteLine("# run time modifications that can affect the use")
			objFile.WriteLine("# of OSSEC. Only change it if you know what you")
			objFile.WriteLine("# are doing. Look first at ossec.conf")
			objFile.WriteLine("# for most of the things you want to change.")
			objFile.WriteLine("#")
			objFile.WriteLine("# This file will not be overwritten during upgrades")
			objFile.WriteLine("# but will be removed when the agent is un-installed.")
			objFile.Close
		End If

	End If

End If

If GetVersion() >= 6 Then
	Set WshShell = CreateObject("WScript.Shell")

	' Remove last backslash from home_dir
	install_dir = Left(home_dir, Len(home_dir) - 1)

	setPermsInherit = "icacls """ & install_dir & """ /inheritancelevel:d"
	WshShell.run setPermsInherit

	remUserPerm = "icacls """ & install_dir & """ /remove *S-1-5-32-545"
	WshShell.run remUserPerm

	' Remove Everyone group for ossec.conf
	remEveryonePerms = "icacls """ & home_dir & "ossec.conf" & """ /remove *S-1-1-0"
	WshShell.run remEveryonePerms
End If

config = 0

End Function

Private Function GetVersion()
	Set objWMIService = GetObject("winmgmts:\\.\root\cimv2")
	Set colItems = objWMIService.ExecQuery("Select * from Win32_OperatingSystem",,48)

	For Each objItem in colItems
		GetVersion = Split(objItem.Version,".")(0)
	Next
End Function

Public Function CheckSvcRunning()
	Set wmi = GetObject("winmgmts://./root/cimv2")
	state = wmi.Get("Win32_Service.Name='OssecSvc'").State
	Session.Property("OSSECRUNNING") = state

	CheckSvcRunning = 0
End Function

'' SIG '' Begin signature block
'' SIG '' MIIvjQYJKoZIhvcNAQcCoIIvfjCCL3oCAQExCzAJBgUr
'' SIG '' DgMCGgUAMGcGCisGAQQBgjcCAQSgWTBXMDIGCisGAQQB
'' SIG '' gjcCAR4wJAIBAQQQTvApFpkntU2P5azhDxfrqwIBAAIB
'' SIG '' AAIBAAIBAAIBADAhMAkGBSsOAwIaBQAEFGtUHf5+F6/Y
'' SIG '' G8psmefMIv7QpEa5oIISIzCCBW8wggRXoAMCAQICEEj8
'' SIG '' k7RgVZSNNqfJionWlBYwDQYJKoZIhvcNAQEMBQAwezEL
'' SIG '' MAkGA1UEBhMCR0IxGzAZBgNVBAgMEkdyZWF0ZXIgTWFu
'' SIG '' Y2hlc3RlcjEQMA4GA1UEBwwHU2FsZm9yZDEaMBgGA1UE
'' SIG '' CgwRQ29tb2RvIENBIExpbWl0ZWQxITAfBgNVBAMMGEFB
'' SIG '' QSBDZXJ0aWZpY2F0ZSBTZXJ2aWNlczAeFw0yMTA1MjUw
'' SIG '' MDAwMDBaFw0yODEyMzEyMzU5NTlaMFYxCzAJBgNVBAYT
'' SIG '' AkdCMRgwFgYDVQQKEw9TZWN0aWdvIExpbWl0ZWQxLTAr
'' SIG '' BgNVBAMTJFNlY3RpZ28gUHVibGljIENvZGUgU2lnbmlu
'' SIG '' ZyBSb290IFI0NjCCAiIwDQYJKoZIhvcNAQEBBQADggIP
'' SIG '' ADCCAgoCggIBAI3nlBIiBCR0Lv8WIwKSirauNoWsR9Qj
'' SIG '' kSs+3H3iMaBRb6yEkeNSirXilt7Qh2MkiYr/7xKTO327
'' SIG '' toq9vQV/J5trZdOlDGmxvEk5mvFtbqrkoIMn2poNK1Dp
'' SIG '' S1uzuGQ2pH5KPalxq2Gzc7M8Cwzv2zNX5b40N+OXG139
'' SIG '' HxI9ggN25vs/ZtKUMWn6bbM0rMF6eNySUPJkx6otBKvD
'' SIG '' aurgL6en3G7X6P/aIatAv7nuDZ7G2Z6Z78beH6kMdrMn
'' SIG '' IKHWuv2A5wHS7+uCKZVwjf+7Fc/+0Q82oi5PMpB0RmtH
'' SIG '' NRN3BTNPYy64LeG/ZacEaxjYcfrMCPJtiZkQsa3bPizk
'' SIG '' qhiwxgcBdWfebeljYx42f2mJvqpFPm5aX4+hW8udMIYw
'' SIG '' 6AOzQMYNDzjNZ6hTiPq4MGX6b8fnHbGDdGk+rMRoO7Hm
'' SIG '' ZzOatgjggAVIQO72gmRGqPVzsAaV8mxln79VWxycVxrH
'' SIG '' eEZ8cKqUG4IXrIfptskOgRxA1hYXKfxcnBgr6kX1773V
'' SIG '' Z08oXgXukEx658b00Pz6zT4yRhMgNooE6reqB0acDZM6
'' SIG '' CWaZWFwpo7kMpjA4PNBGNjV8nLruw9X5Cnb6fgUbQMqS
'' SIG '' NenVetG1fwCuqZCqxX8BnBCxFvzMbhjcb2L+plCnuHu4
'' SIG '' nRU//iAMdcgiWhOVGZAA6RrVwobx447sX/TlAgMBAAGj
'' SIG '' ggESMIIBDjAfBgNVHSMEGDAWgBSgEQojPpbxB+zirynv
'' SIG '' gqV/0DCktDAdBgNVHQ4EFgQUMuuSmv81lkgvKEBCcCA2
'' SIG '' kVwXheYwDgYDVR0PAQH/BAQDAgGGMA8GA1UdEwEB/wQF
'' SIG '' MAMBAf8wEwYDVR0lBAwwCgYIKwYBBQUHAwMwGwYDVR0g
'' SIG '' BBQwEjAGBgRVHSAAMAgGBmeBDAEEATBDBgNVHR8EPDA6
'' SIG '' MDigNqA0hjJodHRwOi8vY3JsLmNvbW9kb2NhLmNvbS9B
'' SIG '' QUFDZXJ0aWZpY2F0ZVNlcnZpY2VzLmNybDA0BggrBgEF
'' SIG '' BQcBAQQoMCYwJAYIKwYBBQUHMAGGGGh0dHA6Ly9vY3Nw
'' SIG '' LmNvbW9kb2NhLmNvbTANBgkqhkiG9w0BAQwFAAOCAQEA
'' SIG '' Er+h74t0mphEuGlGtaskCgykime4OoG/RYp9UgeojR9O
'' SIG '' IYU5o2teLSCGvxC4rnk7U820+9hEvgbZXGNn1EAWh0SG
'' SIG '' cirWMhX1EoPC+eFdEUBn9kIncsUj4gI4Gkwg4tsB981G
'' SIG '' TyaifGbAUTa2iQJUx/xY+2wA7v6Ypi6VoQxTKR9v2Bmm
'' SIG '' T573rAnqXYLGi6+Ap72BSFKEMdoy7BXkpkw9bDlz1AuF
'' SIG '' OSDghRpo4adIOKnRNiV3wY0ZFsWITGZ9L2POmOhp36w8
'' SIG '' qF2dyRxbrtjzL3TPuH7214OdEZZimq5FE9p/3Ef738NS
'' SIG '' n+YGVemdjPI6YlG87CQPKdRYgITkRXta2DCCBhowggQC
'' SIG '' oAMCAQICEGIdbQxSAZ47kHkVIIkhHAowDQYJKoZIhvcN
'' SIG '' AQEMBQAwVjELMAkGA1UEBhMCR0IxGDAWBgNVBAoTD1Nl
'' SIG '' Y3RpZ28gTGltaXRlZDEtMCsGA1UEAxMkU2VjdGlnbyBQ
'' SIG '' dWJsaWMgQ29kZSBTaWduaW5nIFJvb3QgUjQ2MB4XDTIx
'' SIG '' MDMyMjAwMDAwMFoXDTM2MDMyMTIzNTk1OVowVDELMAkG
'' SIG '' A1UEBhMCR0IxGDAWBgNVBAoTD1NlY3RpZ28gTGltaXRl
'' SIG '' ZDErMCkGA1UEAxMiU2VjdGlnbyBQdWJsaWMgQ29kZSBT
'' SIG '' aWduaW5nIENBIFIzNjCCAaIwDQYJKoZIhvcNAQEBBQAD
'' SIG '' ggGPADCCAYoCggGBAJsrnVP6NT+OYAZDasDP9X/2yFNT
'' SIG '' GMjO02x+/FgHlRd5ZTMLER4ARkZsQ3hAyAKwktlQqFZO
'' SIG '' GP/I+rLSJJmFeRno+DYDY1UOAWKA4xjMHY4qF2p9YZWh
'' SIG '' hbeFpPb09JNqFiTCYy/Rv/zedt4QJuIxeFI61tqb7/fo
'' SIG '' XT1/LW2wHyN79FXSYiTxcv+18Irpw+5gcTbXnDOsrSHV
'' SIG '' JYdPE9s+5iRF2Q/TlnCZGZOcA7n9qudjzeN43OE/TpKF
'' SIG '' 2dGq1mVXn37zK/4oiETkgsyqA5lgAQ0c1f1IkOb6rGnh
'' SIG '' WqkHcxX+HnfKXjVodTmmV52L2UIFsf0l4iQ0UgKJUc2R
'' SIG '' GarhOnG3B++OxR53LPys3J9AnL9o6zlviz5pzsgfrQH4
'' SIG '' lrtNUz4Qq/Va5MbBwuahTcWk4UxuY+PynPjgw9nV/35g
'' SIG '' RAhC3L81B3/bIaBb659+Vxn9kT2jUztrkmep/aLb+4xJ
'' SIG '' bKZHyvahAEx2XKHafkeKtjiMqcUf/2BG935A591Gsllv
'' SIG '' WwIDAQABo4IBZDCCAWAwHwYDVR0jBBgwFoAUMuuSmv81
'' SIG '' lkgvKEBCcCA2kVwXheYwHQYDVR0OBBYEFA8qyyCHKLjs
'' SIG '' b0iuK1SmKaoXpM0MMA4GA1UdDwEB/wQEAwIBhjASBgNV
'' SIG '' HRMBAf8ECDAGAQH/AgEAMBMGA1UdJQQMMAoGCCsGAQUF
'' SIG '' BwMDMBsGA1UdIAQUMBIwBgYEVR0gADAIBgZngQwBBAEw
'' SIG '' SwYDVR0fBEQwQjBAoD6gPIY6aHR0cDovL2NybC5zZWN0
'' SIG '' aWdvLmNvbS9TZWN0aWdvUHVibGljQ29kZVNpZ25pbmdS
'' SIG '' b290UjQ2LmNybDB7BggrBgEFBQcBAQRvMG0wRgYIKwYB
'' SIG '' BQUHMAKGOmh0dHA6Ly9jcnQuc2VjdGlnby5jb20vU2Vj
'' SIG '' dGlnb1B1YmxpY0NvZGVTaWduaW5nUm9vdFI0Ni5wN2Mw
'' SIG '' IwYIKwYBBQUHMAGGF2h0dHA6Ly9vY3NwLnNlY3RpZ28u
'' SIG '' Y29tMA0GCSqGSIb3DQEBDAUAA4ICAQAG/4Lhd2M2bnuh
'' SIG '' FSCbE/8E/ph1RGHDVpVx0ZE/haHrQECxyNbgcv2FymQ5
'' SIG '' PPmNS6Dah66dtgCjBsULYAor5wxxcgEPRl05pZOzI3IE
'' SIG '' Gwwsepp+8iGsLKaVpL3z5CmgELIqmk/Q5zFgR1TSGmxq
'' SIG '' oEEhk60FqONzDn7D8p4W89h8sX+V1imaUb693TGqWp3T
'' SIG '' 32IKGfIgy9jkd7GM7YCa2xulWfQ6E1xZtYNEX/ewGnp9
'' SIG '' ZeHPsNwwviJMBZL4xVd40uPWUnOJUoSiugaz0yWLODRt
'' SIG '' Qxs5qU6E58KKmfHwJotl5WZ7nIQuDT0mWjwEx7zSM7fs
'' SIG '' 9Tx6N+Q/3+49qTtUvAQsrEAxwmzOTJ6Jp6uWmHCgrHW4
'' SIG '' dHM3ITpvG5Ipy62KyqYovk5O6cC+040Si15KJpuQ9VJn
'' SIG '' bPvqYqfMB9nEKX/d2rd1Q3DiuDexMKCCQdJGpOqUsxLu
'' SIG '' COuFOoGbO7Uv3RjUpY39jkkp0a+yls6tN85fJe+Y8voT
'' SIG '' nbPU1knpy24wUFBkfenBa+pRFHwCBB1QtS+vGNRhsceP
'' SIG '' 3kSPNrrfN2sRzFYsNfrFaWz8YOdU254qNZQfd9O/VjxZ
'' SIG '' 2Gjr3xgANHtM3HxfzPYF6/pKK8EE4dj66qKKtm2DTL1K
'' SIG '' FCg/OYJyfrdLJq1q2/HXntgr2GVw+ZWhrWgMTn8v1SjZ
'' SIG '' sLlrgIfZHDCCBo4wggT2oAMCAQICEFJPDvq/5fA1slXw
'' SIG '' UIxFvbEwDQYJKoZIhvcNAQEMBQAwVDELMAkGA1UEBhMC
'' SIG '' R0IxGDAWBgNVBAoTD1NlY3RpZ28gTGltaXRlZDErMCkG
'' SIG '' A1UEAxMiU2VjdGlnbyBQdWJsaWMgQ29kZSBTaWduaW5n
'' SIG '' IENBIFIzNjAeFw0yMjAzMjIwMDAwMDBaFw0yMzAzMjIy
'' SIG '' MzU5NTlaMH8xCzAJBgNVBAYTAkNOMRIwEAYDVQQIDAnl
'' SIG '' m5vlt53nnIExLTArBgNVBAoMJOaIkOmDveaZuuWuieS6
'' SIG '' keW+oee9kee7nOaciemZkOWFrOWPuDEtMCsGA1UEAwwk
'' SIG '' 5oiQ6YO95pm65a6J5LqR5b6h572R57uc5pyJ6ZmQ5YWs
'' SIG '' 5Y+4MIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKC
'' SIG '' AgEAxWBokdRlp8qbLVM8pdzzQmnh5P5+zDKuRu8lfP2x
'' SIG '' CxpRRGcr2aqXgi1QudFxfuV5qKBLxUcnZRZwrPjcNvmL
'' SIG '' vDsrIxv5IlTthVa0qxiR1WZYMRx0OowI5fJK8Zub5Lcc
'' SIG '' 8r70AkibIJi5/3KBdkis+5n8NlDQ2ZMyxvCsU80nDc/m
'' SIG '' EI/N/Sb+AbsDlDKyRefWQkZN/b5Ba4NmQN4cn/fRL8jO
'' SIG '' a4BjWKGxtAm7WdrpLrplkytNSMUIS9b3w3fSgv1NACtj
'' SIG '' XqspwpjPRPrKfyIXkIhSYy0gP9i/5+/of5/sQtR/t09w
'' SIG '' KEsKBrXBabpIBsau6uSPuQW6wQGOWbzFBirClaF9lDvP
'' SIG '' bNotpblMdaak/epXVx0u26ISvfQijzo3/SD7saFNDHU3
'' SIG '' 9dywwxOXexRMHg/6hk65wSACqckbbFj87tMaeqin/hCK
'' SIG '' urnOJ5UNf4KnQ/jhOmQCfVmBEgSqlton3rbuMlcgyLWl
'' SIG '' emz2rlbUmhnd5RVcMqsKZyELlTYOdn2/E0AJjdevX8lh
'' SIG '' XkmUysxjB76pudJQTpkSrFietNH4Hs+FyZTyZsVXOW0f
'' SIG '' aoVDIsp4Z3NrSam61Sooy4AZrwyFFsKebfkdaIIMVFJh
'' SIG '' Y8wGAjaRFP+14NuBnr+toO6VtstgSNcrOSj3GhG3rKkP
'' SIG '' DHHYjHCU/nL6ZqTe6jNQEFdKyekCAwEAAaOCAa8wggGr
'' SIG '' MB8GA1UdIwQYMBaAFA8qyyCHKLjsb0iuK1SmKaoXpM0M
'' SIG '' MB0GA1UdDgQWBBTqEKQ4lPA1006hfbw8hR2I69VovjAO
'' SIG '' BgNVHQ8BAf8EBAMCB4AwDAYDVR0TAQH/BAIwADATBgNV
'' SIG '' HSUEDDAKBggrBgEFBQcDAzBKBgNVHSAEQzBBMDUGDCsG
'' SIG '' AQQBsjEBAgEDAjAlMCMGCCsGAQUFBwIBFhdodHRwczov
'' SIG '' L3NlY3RpZ28uY29tL0NQUzAIBgZngQwBBAEwSQYDVR0f
'' SIG '' BEIwQDA+oDygOoY4aHR0cDovL2NybC5zZWN0aWdvLmNv
'' SIG '' bS9TZWN0aWdvUHVibGljQ29kZVNpZ25pbmdDQVIzNi5j
'' SIG '' cmwweQYIKwYBBQUHAQEEbTBrMEQGCCsGAQUFBzAChjho
'' SIG '' dHRwOi8vY3J0LnNlY3RpZ28uY29tL1NlY3RpZ29QdWJs
'' SIG '' aWNDb2RlU2lnbmluZ0NBUjM2LmNydDAjBggrBgEFBQcw
'' SIG '' AYYXaHR0cDovL29jc3Auc2VjdGlnby5jb20wJAYDVR0R
'' SIG '' BB0wG4EZc2FmZS1wcm9kdWN0QHpoaWFubmV0LmNvbTAN
'' SIG '' BgkqhkiG9w0BAQwFAAOCAYEAOmoWiX1QJXVe0OE+OTcV
'' SIG '' pFWtccEEtw1IVHGFMffOvmbtyQoszf9s5zeS0ae/Mx1X
'' SIG '' 3dzy2rILMm61E2rFvaoDhCC26mtVSjqVcy39+/2YBLbS
'' SIG '' rxxykAFVhwTRJ3Eki5YiUL9vbIgtIFOi9yi/c9NK/Zm0
'' SIG '' banufHaySYpqKlewbJekIcsy5OX7CVW/zxVReHOJbLFX
'' SIG '' 5UqFlXPzCurLTikjp2PhhT2kmPxT12nsSKNgE8RwrUH0
'' SIG '' VvKrpYaQaRpjeskVF3vtllWY/BpCdk+rdBdIDO6Vgjs/
'' SIG '' HFPVLCcP7YHXTW6t2cRJVTKitLJ9cuqpTpnOf9RzoxJD
'' SIG '' kq2yr+zA5YhBKX5C1DmjxoolP3uQp+WrGwVoQp4CNonr
'' SIG '' iAlEZSI50qXbeGWVg1a2Eqq2QtdPYSQPxiK3dlDHrnmn
'' SIG '' P82P2wvdf5XbpLzSHK388RMh4QM859nroykmbsFlYbc4
'' SIG '' 1eTC+D28BUcdaFrrn+Zv6hb0TZxOR0G0QoNGH244BsDm
'' SIG '' tIdyceKAMYIc1jCCHNICAQEwaDBUMQswCQYDVQQGEwJH
'' SIG '' QjEYMBYGA1UEChMPU2VjdGlnbyBMaW1pdGVkMSswKQYD
'' SIG '' VQQDEyJTZWN0aWdvIFB1YmxpYyBDb2RlIFNpZ25pbmcg
'' SIG '' Q0EgUjM2AhBSTw76v+XwNbJV8FCMRb2xMAkGBSsOAwIa
'' SIG '' BQCgcDAQBgorBgEEAYI3AgEMMQIwADAZBgkqhkiG9w0B
'' SIG '' CQMxDAYKKwYBBAGCNwIBBDAcBgorBgEEAYI3AgELMQ4w
'' SIG '' DAYKKwYBBAGCNwIBFTAjBgkqhkiG9w0BCQQxFgQUUv8w
'' SIG '' PK1jkQxoHvSehOvgLM30XSkwDQYJKoZIhvcNAQEBBQAE
'' SIG '' ggIAnYNHcx8TOxnYpuKWXusgkU9kG6dATLRtdAXVqb44
'' SIG '' L1H5aNEyMaPxbLn9YZM2t4OsayS0+EIYGsuAJ2hRXTF4
'' SIG '' TRVBDZoLhuw7t9RpHIYyOU8odqWFHRqYjVzRGcCVBdet
'' SIG '' IEZDTIsBv5oEyieENcfTVqUl4GCzybR14N46E+hT4fwr
'' SIG '' I7oV0D3SIseF4VmcLizYYR0VzKxHok7BqK90rVxQiB4B
'' SIG '' PbTRnjAVll8F0YxaSZKHPOMCvQm0O3hpgpzKnmezSIjR
'' SIG '' gU4m0E498ICa6NdJeDYf34NM+/eO3C0BJxBxTli+0JC3
'' SIG '' s5HKDiU53bsYKJFCVuUuYg+kHg7cpCl5T6NkIi1RycGg
'' SIG '' t33oTVIwHRL5HHMoG4FIwChJ+CICpgKRTWxuW7aOv+xk
'' SIG '' qbyNic+Bq6s3aV9uvLqgkSN4o+CtsZ6VuG2Z9VxuC9gT
'' SIG '' B5kcq9a5xO6z1yK52Siz4VURjGz5zlh64yQlyFTxyUSL
'' SIG '' u8yRgJHeW6hMjFb01J0z8iogOXsXj+FXiKNIMWZ2BWbK
'' SIG '' pWGWQqMhuvpWIXyBPLKLFdRQZconkQpl6LffJB/l4KSA
'' SIG '' EiNmOXmew7egVc8BCjELdwhz7GBFNgAoPAU28CIst8Kv
'' SIG '' ilkChZpkcrezHwsRY+rK0KvRQuld8NEja0BJphPjL+Za
'' SIG '' NFGCpZm0KpbU7myYJoD8hqAVKZChghnRMIIZzQYKKwYB
'' SIG '' BAGCNwMDATGCGb0wghm5BgkqhkiG9w0BBwKgghmqMIIZ
'' SIG '' pgIBAzENMAsGCWCGSAFlAwQCATCB3AYLKoZIhvcNAQkQ
'' SIG '' AQSggcwEgckwgcYCAQEGCSsGAQQBoDICAzAxMA0GCWCG
'' SIG '' SAFlAwQCAQUABCCJJf4L2FMxsA3Ntu0Ae/k8XoS648lO
'' SIG '' s3ZrW58z3ZmoqwIUeNKevKYSH1kiGui1Vl3XZgf1XYYY
'' SIG '' DzIwMjIwMzI0MTAxOTE5WjADAgEBoFekVTBTMQswCQYD
'' SIG '' VQQGEwJCRTEZMBcGA1UECgwQR2xvYmFsU2lnbiBudi1z
'' SIG '' YTEpMCcGA1UEAwwgR2xvYmFsc2lnbiBUU0EgZm9yIEFk
'' SIG '' dmFuY2VkIC0gRzSgghVkMIIGVTCCBD2gAwIBAgIQAQBG
'' SIG '' aVCmBKnZcOgd0k1BnzANBgkqhkiG9w0BAQsFADBbMQsw
'' SIG '' CQYDVQQGEwJCRTEZMBcGA1UEChMQR2xvYmFsU2lnbiBu
'' SIG '' di1zYTExMC8GA1UEAxMoR2xvYmFsU2lnbiBUaW1lc3Rh
'' SIG '' bXBpbmcgQ0EgLSBTSEEzODQgLSBHNDAeFw0yMTA1Mjcw
'' SIG '' OTU1MjNaFw0zMjA2MjgwOTU1MjJaMFMxCzAJBgNVBAYT
'' SIG '' AkJFMRkwFwYDVQQKDBBHbG9iYWxTaWduIG52LXNhMSkw
'' SIG '' JwYDVQQDDCBHbG9iYWxzaWduIFRTQSBmb3IgQWR2YW5j
'' SIG '' ZWQgLSBHNDCCAaIwDQYJKoZIhvcNAQEBBQADggGPADCC
'' SIG '' AYoCggGBAN8waZh7lw1uo1S0OV9kWXUEIv5OaW/oF3R0
'' SIG '' pX1RGA5GIB9oLrrlZdbJ0pGh7KT3Veqq7TvM+2KbhEKS
'' SIG '' ejJM+fTyHPiY0bkxgSWtrVZnMqb+hzLzXMMtYvFNiQw3
'' SIG '' tH/rKuNgi29sPTHy7cldgJspnVXg4sT/6naTGB5sqat7
'' SIG '' yR4SYdXA56Dm+JApMyy4v25ast3HB0PRO9swC7R4w+zq
'' SIG '' 8aJUz2CTOMz3ZEP1zwgEFnDItNsO1AqKCNy7k8EdbvKM
'' SIG '' nOshNZ7/j7ywfsKEOH7mnWR6JqDxILG84dgqJZ0YUuRt
'' SIG '' 1EwwCnjMLUaO7VcLP3mVUKcDsDODMrdAnvS0kpcTDFC3
'' SIG '' nqq0QU4LmInM+8QhRJAyjkjyLEsMF+SEV1umrPuXg/mN
'' SIG '' ZFTC7GpDHs8KdpKyEL/t1qMgD7XRMI4aQLE259COePMT
'' SIG '' wC8LiJA7CGHjD61Hsw5UcJV/oEPUWsbdF5+UywCHaA7h
'' SIG '' rpPuLHIEGzIXkEvXK4AlBR/lM/TowGgqeReg7wIDAQAB
'' SIG '' o4IBmzCCAZcwDgYDVR0PAQH/BAQDAgeAMBYGA1UdJQEB
'' SIG '' /wQMMAoGCCsGAQUFBwMIMB0GA1UdDgQWBBSufnCBeCAU
'' SIG '' Ka3yePhZANnMpiQCjjBMBgNVHSAERTBDMEEGCSsGAQQB
'' SIG '' oDIBHjA0MDIGCCsGAQUFBwIBFiZodHRwczovL3d3dy5n
'' SIG '' bG9iYWxzaWduLmNvbS9yZXBvc2l0b3J5LzAJBgNVHRME
'' SIG '' AjAAMIGQBggrBgEFBQcBAQSBgzCBgDA5BggrBgEFBQcw
'' SIG '' AYYtaHR0cDovL29jc3AuZ2xvYmFsc2lnbi5jb20vY2Ev
'' SIG '' Z3N0c2FjYXNoYTM4NGc0MEMGCCsGAQUFBzAChjdodHRw
'' SIG '' Oi8vc2VjdXJlLmdsb2JhbHNpZ24uY29tL2NhY2VydC9n
'' SIG '' c3RzYWNhc2hhMzg0ZzQuY3J0MB8GA1UdIwQYMBaAFOoW
'' SIG '' xmnn48tXRTkzpPBAvtDDvWWWMEEGA1UdHwQ6MDgwNqA0
'' SIG '' oDKGMGh0dHA6Ly9jcmwuZ2xvYmFsc2lnbi5jb20vY2Ev
'' SIG '' Z3N0c2FjYXNoYTM4NGc0LmNybDANBgkqhkiG9w0BAQsF
'' SIG '' AAOCAgEAf2Lo+tl3L0Jvaw/X3UVZPPR1egDsvfZvDiNt
'' SIG '' LTNCchRPRJSBveuMAohMrH/HXc23xCSau5kBaApa6kVh
'' SIG '' 07As132gF+5dgPEa4uf8sd8dMgQoDzaE1wlGLbZ+wEAV
'' SIG '' Ihp5YWeXthKP0E9mLC5UKlgGrJlO/XWtVCYKaP+SJ/g8
'' SIG '' uRltMIEmTIUs83Pcj+DlymRKe0cRTNqi1Lfx5FF65jmw
'' SIG '' IQcZ4PCMuXFwfZHtNJ+LMZ4NxMY+Nitm1sBB1bIjSSLT
'' SIG '' vl+JNoxa1sVQqj8OTlQJtv4Nkdlx2J82PDSOiYO35PNm
'' SIG '' Ss43kItdeuo+o+MHBz2UGRSe+rFnS+u2srcIb5KWRV1M
'' SIG '' 7g5ZWotmc2FFNkqGzmNDGW4GOglGOZB0xnMLXkLRzS8i
'' SIG '' bCQnpwICUZKNAbRdhcf4w0F13WSM8vOY7um3hwmnvQoT
'' SIG '' MDdiH1nnKXJ3aXV4kLDNHDpcahCGcvcAsjKXWXieTviz
'' SIG '' Zv2vK/yJtnWilAo3khNBdd31Pzqup6i0QtPZnFES8vJ6
'' SIG '' 1ivsnkwl2W2ckfQfAU9Ix+yP+Vuq7PpcEXJgruw3cZS+
'' SIG '' XEmJTClt81c7GgBXvL6QLkJhgtXf/wCBlnwBVZO4YmTo
'' SIG '' BoarVUpvM8Xz2lgFjd0B9TxVIYX+ezV5xX+y+9itvZ35
'' SIG '' VQokZHRhiiuXNl9WvfLX4Ox8/fnrktQwggZZMIIEQaAD
'' SIG '' AgECAg0B7BySQN79LkBdfEd0MA0GCSqGSIb3DQEBDAUA
'' SIG '' MEwxIDAeBgNVBAsTF0dsb2JhbFNpZ24gUm9vdCBDQSAt
'' SIG '' IFI2MRMwEQYDVQQKEwpHbG9iYWxTaWduMRMwEQYDVQQD
'' SIG '' EwpHbG9iYWxTaWduMB4XDTE4MDYyMDAwMDAwMFoXDTM0
'' SIG '' MTIxMDAwMDAwMFowWzELMAkGA1UEBhMCQkUxGTAXBgNV
'' SIG '' BAoTEEdsb2JhbFNpZ24gbnYtc2ExMTAvBgNVBAMTKEds
'' SIG '' b2JhbFNpZ24gVGltZXN0YW1waW5nIENBIC0gU0hBMzg0
'' SIG '' IC0gRzQwggIiMA0GCSqGSIb3DQEBAQUAA4ICDwAwggIK
'' SIG '' AoICAQDwAuIwI/rgG+GadLOvdYNfqUdSx2E6Y3w5I3lt
'' SIG '' dPwx5HQSGZb6zidiW64HiifuV6PENe2zNMeswwzrgGZt
'' SIG '' 0ShKwSy7uXDycq6M95laXXauv0SofEEkjo+6xU//NkGr
'' SIG '' py39eE5DiP6TGRfZ7jHPvIo7bmrEiPDul/bc8xigS5kc
'' SIG '' DoenJuGIyaDlmeKe9JxMP11b7Lbv0mXPRQtUPbFUUweL
'' SIG '' mW64VJmKqDGSO/J6ffwOWN+BauGwbB5lgirUIceU/kKW
'' SIG '' O/ELsX9/RpgOhz16ZevRVqkuvftYPbWF+lOZTVt07XJL
'' SIG '' og2CNxkM0KvqWsHvD9WZuT/0TzXxnA/TNxNS2SU07Zbv
'' SIG '' +GfqCL6PSXr/kLHU9ykV1/kNXdaHQx50xHAotIB7vSqb
'' SIG '' u4ThDqxvDbm19m1W/oodCT4kDmcmx/yyDaCUsLKUzHvm
'' SIG '' Z/6mWLLU2EESwVX9bpHFu7FMCEue1EIGbxsY1TbqZK7O
'' SIG '' /fUF5uJm0A4FIayxEQYjGeT7BTRE6giunUlnEYuC5a1a
'' SIG '' hqdm/TMDAd6ZJflxbumcXQJMYDzPAo8B/XLukvGnEt5C
'' SIG '' Ek3sqSbldwKsDlcMCdFhniaI/MiyTdtk8EWfusE/VKPY
'' SIG '' dgKVbGqNyiJc9gwE4yn6S7Ac0zd0hNkdZqs0c48efXxe
'' SIG '' ltY9GbCX6oxQkW2vV4Z+EDcdaxoU3wIDAQABo4IBKTCC
'' SIG '' ASUwDgYDVR0PAQH/BAQDAgGGMBIGA1UdEwEB/wQIMAYB
'' SIG '' Af8CAQAwHQYDVR0OBBYEFOoWxmnn48tXRTkzpPBAvtDD
'' SIG '' vWWWMB8GA1UdIwQYMBaAFK5sBaOTE+Ki5+LXHNbH8H/I
'' SIG '' Z1OgMD4GCCsGAQUFBwEBBDIwMDAuBggrBgEFBQcwAYYi
'' SIG '' aHR0cDovL29jc3AyLmdsb2JhbHNpZ24uY29tL3Jvb3Ry
'' SIG '' NjA2BgNVHR8ELzAtMCugKaAnhiVodHRwOi8vY3JsLmds
'' SIG '' b2JhbHNpZ24uY29tL3Jvb3QtcjYuY3JsMEcGA1UdIARA
'' SIG '' MD4wPAYEVR0gADA0MDIGCCsGAQUFBwIBFiZodHRwczov
'' SIG '' L3d3dy5nbG9iYWxzaWduLmNvbS9yZXBvc2l0b3J5LzAN
'' SIG '' BgkqhkiG9w0BAQwFAAOCAgEAf+KI2VdnK0JfgacJC7rE
'' SIG '' uygYVtZMv9sbB3DG+wsJrQA6YDMfOcYWaxlASSUIHuSb
'' SIG '' 99akDY8elvKGohfeQb9P4byrze7AI4zGhf5LFST5GETs
'' SIG '' H8KkrNCyz+zCVmUdvX/23oLIt59h07VGSJiXAmd6FpVK
'' SIG '' 22LG0LMCzDRIRVXd7OlKn14U7XIQcXZw0g+W8+o3V5SR
'' SIG '' GK/cjZk4GVjCqaF+om4VJuq0+X8q5+dIZGkv0pqhcvb3
'' SIG '' JEt0Wn1yhjWzAlcfi5z8u6xM3vreU0yD/RKxtklVT3Wd
'' SIG '' rG9KyC5qucqIwxIwTrIIc59eodaZzul9S5YszBZrGM3k
'' SIG '' WTeGCSziRdayzW6CdaXajR63Wy+ILj198fKRMAWcznt8
'' SIG '' oMWsr1EG8BHHHTDFUVZg6HyVPSLj1QokUyeXgPpIiScs
'' SIG '' eeI85Zse46qEgok+wEr1If5iEO0dMPz2zOpIJ3yLdUJ/
'' SIG '' a8vzpWuVHwRYNAqJ7YJQ5NF7qMnmvkiqK1XZjbclIA4b
'' SIG '' UaDUY6qD6mxyYUrJ+kPExlfFnbY8sIuwuRwx773vFNgU
'' SIG '' QGwgHcIt6AvGjW2MtnHtUiH+PvafnzkarqzSL3ogsfSs
'' SIG '' qh3iLRSd+pZqHcY8yvPZHL9TTaRHWXyVxENB+SXiLBB+
'' SIG '' gfkNlKd98rUJ9dhgckBQlSDUQ0S++qCV5yBZtnjGpGqq
'' SIG '' IpswggVHMIIEL6ADAgECAg0B8kBCQM79ItvpbHH8MA0G
'' SIG '' CSqGSIb3DQEBDAUAMEwxIDAeBgNVBAsTF0dsb2JhbFNp
'' SIG '' Z24gUm9vdCBDQSAtIFIzMRMwEQYDVQQKEwpHbG9iYWxT
'' SIG '' aWduMRMwEQYDVQQDEwpHbG9iYWxTaWduMB4XDTE5MDIy
'' SIG '' MDAwMDAwMFoXDTI5MDMxODEwMDAwMFowTDEgMB4GA1UE
'' SIG '' CxMXR2xvYmFsU2lnbiBSb290IENBIC0gUjYxEzARBgNV
'' SIG '' BAoTCkdsb2JhbFNpZ24xEzARBgNVBAMTCkdsb2JhbFNp
'' SIG '' Z24wggIiMA0GCSqGSIb3DQEBAQUAA4ICDwAwggIKAoIC
'' SIG '' AQCVB+hzymb57BTKezz3DQjxtEULLIK0SMbrWzyug7hB
'' SIG '' kjMUpG9/6SrMxrCIa8W2idHGsv8UzlEUIexK3RtaxtaH
'' SIG '' 7k06FQbtZGYLkoDKRN5zlE7zp4l/T3hjCMgSUG1CZi9N
'' SIG '' uXkoTVIaihqAtxmBDn7EirxkTCEcQ2jXPTyKxbJm1ZCa
'' SIG '' tzEGxb7ibTIGph75ueuqo7i/voJjUNDwGInf5A959eqi
'' SIG '' HyrScC5757yTu21T4kh8jBAHOP9msndhfuDqjDyqtKT2
'' SIG '' 85VKEgdt/Yyyic/QoGF3yFh0sNQjOvddOsqi250J3l1E
'' SIG '' LZDxgc1Xkvp+vFAEYzTfa5MYvms2sjnkrCQ2t/DvthwT
'' SIG '' V5O23rL44oW3c6K4NapF8uCdNqFvVIrxclZuLojFUUJE
'' SIG '' FZTuo8U4lptOTloLR/MGNkl3MLxxN+Wm7CEIdfzmYRY/
'' SIG '' d9XZkZeECmzUAk10wBTt/Tn7g/JeFKEEsAvp/u6P4W4L
'' SIG '' sgizYWYJarEGOmWWWcDwNf3J2iiNGhGHcIEKqJp1HZ46
'' SIG '' hgUAntuA1iX53AWeJ1lMdjlb6vmlodiDD9H/3zAR+YXP
'' SIG '' M0j1ym1kFCx6WE/TSwhJxZVkGmMOeT31s4zKWK2cQkV5
'' SIG '' bg6HGVxUsWW2v4yb3BPpDW+4LtxnbsmLEbWEFIoAGXCD
'' SIG '' eZGXkdQaJ783HjIH2BRjPChMrwIDAQABo4IBJjCCASIw
'' SIG '' DgYDVR0PAQH/BAQDAgEGMA8GA1UdEwEB/wQFMAMBAf8w
'' SIG '' HQYDVR0OBBYEFK5sBaOTE+Ki5+LXHNbH8H/IZ1OgMB8G
'' SIG '' A1UdIwQYMBaAFI/wS3+oLkUkrk1Q+mOai97i3Ru8MD4G
'' SIG '' CCsGAQUFBwEBBDIwMDAuBggrBgEFBQcwAYYiaHR0cDov
'' SIG '' L29jc3AyLmdsb2JhbHNpZ24uY29tL3Jvb3RyMzA2BgNV
'' SIG '' HR8ELzAtMCugKaAnhiVodHRwOi8vY3JsLmdsb2JhbHNp
'' SIG '' Z24uY29tL3Jvb3QtcjMuY3JsMEcGA1UdIARAMD4wPAYE
'' SIG '' VR0gADA0MDIGCCsGAQUFBwIBFiZodHRwczovL3d3dy5n
'' SIG '' bG9iYWxzaWduLmNvbS9yZXBvc2l0b3J5LzANBgkqhkiG
'' SIG '' 9w0BAQwFAAOCAQEASaxexYPzWsthKk2XShUpn+QUkKoJ
'' SIG '' +cR6nzUYigozFW1yhyJOQT9tCp4YrtviX/yV0SyYFDuO
'' SIG '' wfA2WXnzjYHPdPYYpOThaM/vf2VZQunKVTm808Um7nE4
'' SIG '' +tchAw+3TtlbYGpDtH0J0GBh3artAF5OMh7gsmyePLLC
'' SIG '' u5jTkHZqaa0a3KiJ2lhP0sKLMkrOVPs46TsHC3UKEdsL
'' SIG '' fCUn8awmzxFT5tzG4mE1MvTO3YPjGTrrwmijcgDIJDxO
'' SIG '' uFM8sRer5jUs+dNCKeZfYAOsQmGmsVdqM0LfNTGGyj43
'' SIG '' K9rE2iT1ThLytrm3R+q7IK1hFregM+Mtiae8szwBfyMa
'' SIG '' gAk06TCCA18wggJHoAMCAQICCwQAAAAAASFYUwiiMA0G
'' SIG '' CSqGSIb3DQEBCwUAMEwxIDAeBgNVBAsTF0dsb2JhbFNp
'' SIG '' Z24gUm9vdCBDQSAtIFIzMRMwEQYDVQQKEwpHbG9iYWxT
'' SIG '' aWduMRMwEQYDVQQDEwpHbG9iYWxTaWduMB4XDTA5MDMx
'' SIG '' ODEwMDAwMFoXDTI5MDMxODEwMDAwMFowTDEgMB4GA1UE
'' SIG '' CxMXR2xvYmFsU2lnbiBSb290IENBIC0gUjMxEzARBgNV
'' SIG '' BAoTCkdsb2JhbFNpZ24xEzARBgNVBAMTCkdsb2JhbFNp
'' SIG '' Z24wggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIB
'' SIG '' AQDMJXaQeQZ4Ihb1wIO2hMoonv0FdhHFrYhy/EYCQ8ey
'' SIG '' ip0EXyTLLkvhYIJG4VKrDIFHcGzdZNHr9SyjD4I9DCuu
'' SIG '' l9e2FIYQebs7E4B3jAjhSdJqYi8fXvqWaN+JJ5U4nwbX
'' SIG '' PsnLJlkNc96wyOkmDoMVxu9bi9IEYMpJpij2aTv2y8go
'' SIG '' keWdimFXN6x0FNx04Druci8unPvQu7/1PQDhBjPogiuu
'' SIG '' U6Y6FnOM3UEOIDrAtKeh6bJPkC4yYOlXy7kEkmho5Tgm
'' SIG '' YHWyn3f/kRTvriBJ/K1AFUjRAjFhGV64l++td7dkmnq/
'' SIG '' X8ET75ti+w1s4FRpFqkD2m7pg5NxdsZphYIXAgMBAAGj
'' SIG '' QjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNVHRMBAf8EBTAD
'' SIG '' AQH/MB0GA1UdDgQWBBSP8Et/qC5FJK5NUPpjmove4t0b
'' SIG '' vDANBgkqhkiG9w0BAQsFAAOCAQEAS0DbwFCq/sgM7/eW
'' SIG '' VEVJu5YACUGssxOGhigHM8pr5nS5ugAtrqQK0/Xx8Q+K
'' SIG '' v3NnSoPHRHt44K9ubG8DKY4zOUXDjuS5V2yq/BKW7FPG
'' SIG '' LeQkbLmUY/vcU2hnVj6DuM81IcPJaP7O2sJTqsyQiunw
'' SIG '' XUaMld16WCgaLx3ezQA3QY/tRG3XUyiXfvNnBB4V14qW
'' SIG '' tNPeTCekTBtzc3b0F5nCH3oO4y0IrQocLP88q1UOD5F+
'' SIG '' NuvDV0m+4S4tfGCLw0FREyOdzvcya5QBqJnnLDMfOjsl
'' SIG '' 0oZAzjsshnjJYS8Uuu7bVW/fhO4FCU29KNhyztNiUGUe
'' SIG '' 65KXgzHZs7XKR1g/XzGCA0kwggNFAgEBMG8wWzELMAkG
'' SIG '' A1UEBhMCQkUxGTAXBgNVBAoTEEdsb2JhbFNpZ24gbnYt
'' SIG '' c2ExMTAvBgNVBAMTKEdsb2JhbFNpZ24gVGltZXN0YW1w
'' SIG '' aW5nIENBIC0gU0hBMzg0IC0gRzQCEAEARmlQpgSp2XDo
'' SIG '' HdJNQZ8wCwYJYIZIAWUDBAIBoIIBLTAaBgkqhkiG9w0B
'' SIG '' CQMxDQYLKoZIhvcNAQkQAQQwKwYJKoZIhvcNAQk0MR4w
'' SIG '' HDALBglghkgBZQMEAgGhDQYJKoZIhvcNAQELBQAwLwYJ
'' SIG '' KoZIhvcNAQkEMSIEILFGcXyI6qta7RylGSMkX1keSDbl
'' SIG '' dqNcbaS+LIbKFF1XMIGwBgsqhkiG9w0BCRACLzGBoDCB
'' SIG '' nTCBmjCBlwQgE9bpxCD/bU4nVHKMaOd4gmVkZ9uaGQ+B
'' SIG '' ZZf2f7bMxvkwczBfpF0wWzELMAkGA1UEBhMCQkUxGTAX
'' SIG '' BgNVBAoTEEdsb2JhbFNpZ24gbnYtc2ExMTAvBgNVBAMT
'' SIG '' KEdsb2JhbFNpZ24gVGltZXN0YW1waW5nIENBIC0gU0hB
'' SIG '' Mzg0IC0gRzQCEAEARmlQpgSp2XDoHdJNQZ8wDQYJKoZI
'' SIG '' hvcNAQELBQAEggGApj6b3GuQQsrqse2kVeVCODPxBP0B
'' SIG '' nfuTP/RDeGbzupbjCuSga/5aK32iiLBrdRrgI9N76ZZB
'' SIG '' oWr1ym9o7/WryB7VWz0T7EZGoXmeOZ3iLvaL2x0DT4Ub
'' SIG '' HzEfOt8vhHsw2VYv1z8m2hnGZ+X7atfTP/bFv4frt8d+
'' SIG '' LBqm+qqHvKTS66J4FJdphUKpTPdz2HfoQC23B5s6dX9Q
'' SIG '' 7YvShb1nxHQHHwcCTF5yhrW8BHg4/MEhEiF0CU29rVeM
'' SIG '' 8MGHip59vWTA936rY4fBU6EF7lQc1S/MQ8pMHq/HHZoc
'' SIG '' 7WIq1S//BFaL2t1PZ/QjlNbu6VhEU3uCxRZabRjEGCbm
'' SIG '' RGW9USKUgNFLf+XRcgZcF2RfoBUx6TPV2b53z791Hu0v
'' SIG '' KxNGMARBbj/KtuarVlbAo5MvZF3p3R6krO5DjjBLyI7K
'' SIG '' Wn5MUlf7aTou0Ey65rJpjTsONqqlVrtbEBfBBJbcU36h
'' SIG '' fGDkQVmIHTqxRDRMD4nly3vrjlOzJ/CpMcHsqDDkHwhE
'' SIG '' End signature block

import 'package:flutter/material.dart';
import 'package:mobile_scanner/mobile_scanner.dart';
import 'package:dartssh2/dartssh2.dart';

class QRSSHConnectionPage extends StatefulWidget {
  const QRSSHConnectionPage({super.key});

  @override
  State<QRSSHConnectionPage> createState() => _QRSSHConnectionPageState();
}

class _QRSSHConnectionPageState extends State<QRSSHConnectionPage> {
  final MobileScannerController scannerController = MobileScannerController();
  SSHClient? sshClient;
  String statusMessage = 'Aim the camera at the QR code';
  ConnectionState connectionState = ConnectionState.disconnected;
  String? scannedData;
  @override
  void dispose() {
    scannerController.dispose();
    // Não desconectamos o SSH aqui para permitir que a conexão continue ativa
    // mesmo após sair desta tela
    super.dispose();
  }

  Future<void> _connect(String sshCredentials) async {
    setState(() {
      connectionState = ConnectionState.connecting;
      statusMessage = 'Connecting...';
    });

    try {
      final parts = sshCredentials.split('@');
      if (parts.length != 2) throw FormatException('Invalid format');

      final userPass = parts[0].split(':');
      if (userPass.length != 2) throw FormatException('Invalid credentials');

      final ipPort = parts[1].split(':');
      if (ipPort.length != 2)
        throw FormatException(
          'Invalid IP',
        ); // Configura uma conexão com timeout aumentado (30 segundos)
      final socket = await SSHSocket.connect(
        ipPort[0],
        int.parse(ipPort[1]),
        timeout: const Duration(seconds: 30),
      );

      // Cria o cliente SSH com configuração para manter a conexão ativa
      final client = SSHClient(
        socket,
        username: userPass[0],
        onPasswordRequest: () => userPass[1],
        keepAliveInterval: const Duration(
          seconds: 30,
        ), // Envia pacotes keepalive a cada 30 segundos
      );

      // Testa a conexão e configura o ambiente
      await client.execute('echo "Connection test"');

      setState(() {
        connectionState = ConnectionState.connected;
        statusMessage = 'Connected to ${ipPort[0]}';
        sshClient = client;
      });

      if (mounted) {
        Navigator.pop(context, client);
      }
    } catch (e) {
      setState(() {
        connectionState = ConnectionState.error;
        statusMessage = 'Connection failed: ${e.toString()}';
      });
      await _disconnectSSH();
    }
  }

  Future<SSHSession?> _executeRemoteCommand(String command) async {
    if (sshClient == null) return null;
    try {
      final result = await sshClient!.execute(command);
      return result;
    } catch (e) {
      if (mounted) {
        setState(() {
          statusMessage = 'Command failed: ${e.toString()}';
        });
      }
      return null;
    }
  }

  Future<void> _disconnectSSH() async {
    try {
      // Paramos apenas os processos Python, mas mantemos a conexão SSH ativa
      // para uso posterior na aplicação
      await _executeRemoteCommand('pkill -f "python3"');

      // Apenas atualizamos o estado visual, sem fechar a conexão
      if (mounted) {
        setState(() {
          connectionState = ConnectionState.disconnected;
          statusMessage = 'Disconnected';
          // Note que não definimos sshClient como null
          // nem fechamos a conexão com sshClient?.close()
        });
      }
    } catch (e) {
      if (mounted) {
        setState(() {
          statusMessage = 'Disconnection failed: ${e.toString()}';
        });
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Connection'),
        actions: [
          if (connectionState == ConnectionState.connected)
            IconButton(
              icon: const Icon(Icons.check),
              onPressed:
                  () => Navigator.pop(
                    context,
                    scannedData?.split('@')[1].split(':')[0],
                  ),
            ),
        ],
      ),
      body: Column(
        children: [
          Expanded(
            child: Column(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                if (connectionState == ConnectionState.disconnected ||
                    connectionState == ConnectionState.error)
                  SizedBox(
                    height: 300,
                    child: MobileScanner(
                      controller: scannerController,
                      onDetect: (capture) {
                        final barcodes = capture.barcodes;
                        if (barcodes.isNotEmpty &&
                            barcodes.first.rawValue != null) {
                          final data = barcodes.first.rawValue!;
                          setState(() => scannedData = data);
                          _connect(data);
                        }
                      },
                    ),
                  ),
                const SizedBox(height: 20),
                _buildStatusIndicator(),
                Text(
                  statusMessage,
                  style: Theme.of(context).textTheme.titleMedium,
                  textAlign: TextAlign.center,
                ),
              ],
            ),
          ),
          Padding(
            padding: const EdgeInsets.all(20.0),
            child: ElevatedButton(
              onPressed:
                  connectionState == ConnectionState.connected
                      ? _disconnectSSH
                      : null,
              style: ElevatedButton.styleFrom(
                backgroundColor: Colors.red,
                minimumSize: const Size(double.infinity, 50),
              ),
              child: const Text('DISCONNECT'),
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildStatusIndicator() {
    IconData icon;
    Color color;

    switch (connectionState) {
      case ConnectionState.connected:
        icon = Icons.check_circle;
        color = Colors.green;
        break;
      case ConnectionState.connecting:
        icon = Icons.autorenew;
        color = Colors.orange;
        break;
      case ConnectionState.error:
        icon = Icons.error;
        color = Colors.red;
        break;
      default:
        icon = Icons.qr_code_scanner;
        color = Colors.blue;
    }

    return Icon(icon, size: 60, color: color);
  }
}

enum ConnectionState { disconnected, connecting, connected, error }

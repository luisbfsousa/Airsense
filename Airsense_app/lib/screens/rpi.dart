import 'package:dartssh2/dartssh2.dart';
import 'package:flutter/material.dart';
import '../services/qr_scanner.dart';
import 'dart:async';

class RpiPage extends StatefulWidget {
  const RpiPage({super.key});

  @override
  State<RpiPage> createState() => _RpiPageState();
}

Widget _buildActionSquare(
  BuildContext context, {
  required IconData icon,
  required Color icon_color,
  required String label,
  required VoidCallback onTap,
  bool enabled = true,
}) {
  return GestureDetector(
    onTap: enabled ? onTap : null,
    child: Container(
      width: 150,
      height: 150,
      decoration: BoxDecoration(
        color:
            enabled
                ? Colors.blue.withOpacity(0.1)
                : Colors.grey.withOpacity(0.1),
        borderRadius: BorderRadius.circular(20),
        border: Border.all(
          color: enabled ? Colors.blue : Colors.grey,
          width: 2,
        ),
      ),
      child: Column(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          Icon(icon, size: 50, color: enabled ? icon_color : Colors.grey),
          const SizedBox(height: 10),
          Text(
            label,
            style: TextStyle(
              fontSize: 20,
              fontWeight: FontWeight.bold,
              color: enabled ? Colors.black : Colors.grey,
            ),
          ),
        ],
      ),
    ),
  );
}

class _RpiPageState extends State<RpiPage> {
  String? currentIp;
  SSHClient? sshClient;
  bool isConnected = false;
  Timer? _keepAliveTimer;

  @override
  void initState() {
    super.initState();
    _keepAliveTimer = Timer.periodic(const Duration(minutes: 2), (timer) {
      _checkConnection();
    });
  }

  @override
  void dispose() {
    _keepAliveTimer?.cancel();
    super.dispose();
  }

  Future<void> _checkConnection() async {
    if (sshClient != null) {
      try {
        // Enviar um comando simples para verificar se a conexão ainda está ativa
        await _executeRemoteCommand('echo "keep_alive"');
        // Se chegou aqui, a conexão está OK
      } catch (e) {
        // Se houve erro, a conexão foi perdida
        setState(() {
          isConnected = false;
          sshClient = null;
        });

        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(
            content: Text('Conexão SSH perdida. Reconecte novamente.'),
          ),
        );
      }
    }
  }

  Future<SSHSession?> _executeRemoteCommand(String command) async {
    if (sshClient == null) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('Not connected to Raspberry Pi')),
      );
      return null;
    }

    try {
      //print('Executando comando: $command');

      final result = await sshClient!.execute(command);

      // Opcional: mostrar o resultado do comando em um snackbar se desejar feedback visual
      // ScaffoldMessenger.of(context).showSnackBar(
      //   SnackBar(content: Text('Comando executado: $command')),
      // );

      return result;
    } catch (e) {
      ScaffoldMessenger.of(
        context,
      ).showSnackBar(SnackBar(content: Text('Erro ao executar comando: $e')));
      return null;
    }
  }

  Future<void> _rebootRaspberryPi() async {
    if (sshClient == null) return;

    try {
      final confirm = await showDialog<bool>(
        context: context,
        builder:
            (context) => AlertDialog(
              title: const Text('Reboot Raspberry Pi'),
              content: const Text(
                'Are you sure you want to reboot the Raspberry Pi?',
              ),
              actions: [
                TextButton(
                  onPressed: () => Navigator.pop(context, false),
                  child: const Text('Cancel'),
                ),
                TextButton(
                  onPressed: () => Navigator.pop(context, true),
                  child: const Text('Reboot'),
                ),
              ],
            ),
      );

      if (confirm == true) {
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(content: Text('Rebooting Raspberry Pi...')),
        );

        await _executeRemoteCommand('sudo -n reboot');
        sshClient?.close();

        setState(() {
          sshClient = null;
          isConnected = false;
        });
      }
    } catch (e) {
      ScaffoldMessenger.of(
        context,
      ).showSnackBar(SnackBar(content: Text('Error turning off: $e')));
    }
  }

  Future<void> _startscripts() async {
    if (sshClient == null) return;

    setState(() {
      isConnected = true;
    });
    await _executeRemoteCommand('cd /home/airsense && bash ./startBle.sh &');

    Future.delayed(const Duration(seconds: 10), () async {
      if (isConnected && mounted) {
        await _executeRemoteCommand(
          'cd /home/airsense/data_handler && bash ./start.sh &',
        );
      }
    });
  }

  Future<void> _stopscripts() async {
    if (sshClient == null) return;

    setState(() {
      isConnected = false;
    });

    await _executeRemoteCommand('pkill -f "python"');
  }


  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: Center(
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            _buildActionSquare(
              context,
              icon: Icons.qr_code_scanner,
              icon_color: Colors.blue,
              label: 'Connection',
              onTap: () async {
                final client = await Navigator.push<SSHClient>(
                  context,
                  MaterialPageRoute(
                    builder: (context) => const QRSSHConnectionPage(),
                  ),
                );
                if (client != null && mounted) {
                  setState(() => sshClient = client);
                }
              },
            ),

            const SizedBox(height: 40),

            Row(
              mainAxisAlignment: MainAxisAlignment.spaceEvenly,
              children: [
                _buildActionSquare(
                  context,
                  icon: Icons.play_arrow,
                  icon_color: Colors.green,
                  label: 'Start',
                  onTap: () {
                    _startscripts();
                  },
                  enabled: sshClient != null && !isConnected,
                ),
                _buildActionSquare(
                  context,
                  icon: Icons.stop,
                  icon_color: Colors.red,
                  label: 'Stop',
                  onTap: () {
                    _stopscripts();
                  },
                  enabled: sshClient != null && isConnected,
                ),
              ],
            ),

            const SizedBox(height: 20),
            Row(
              mainAxisAlignment: MainAxisAlignment.spaceEvenly,
              children: [
                _buildActionSquare(
                  context,
                  icon: Icons.clear,
                  icon_color: Colors.green,
                  label: 'Clear DB',
                  onTap: () {
                    _executeRemoteCommand(
                      'cd /home/airsense/data_handler && bash ./clear.sh',
                    );
                  },
                  enabled: sshClient != null,
                ),
                _buildActionSquare(
              context,
              icon: Icons.power_settings_new,
              icon_color: Colors.purple,
              label: 'Reboot',
              onTap: () {
                if (sshClient != null) {
                  _rebootRaspberryPi();
                }
              },
              enabled: sshClient != null,
            ),
              ],
            ),
          ],
        ),
      ),
    );
  }
}

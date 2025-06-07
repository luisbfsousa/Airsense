import 'package:flutter/material.dart';
import 'package:http/http.dart' as http;

class SensorhubPage extends StatefulWidget {
  final String? esp32IP;

  const SensorhubPage({required this.esp32IP, Key? key}) : super(key: key);

  @override
  State<SensorhubPage> createState() => _SensorhubPageState();
}

Widget _buildActionSquare(
  BuildContext context, {
  required IconData icon,
  required Color icon_color,
  required String label,
  //required Color color,
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

class _SensorhubPageState extends State<SensorhubPage> {
  bool _isRunning = true;
  bool _isLoading = false;

  Future<void> _sendCommand(String command) async {
    setState(() => _isLoading = true);

    try {
      final response = await http.get(
        Uri.parse('http://${widget.esp32IP}/$command'),
      );
      if (response.statusCode == 200 || response.statusCode == 303) {
        setState(() {
          _isRunning = command == 'start';
        });
      }
    } catch (e) {
      ScaffoldMessenger.of(
        context,
      ).showSnackBar(SnackBar(content: Text('Error: $e')));
    } finally {
      setState(() => _isLoading = false);
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: Center(
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            Text(
              'Code is currently:',
              style: Theme.of(context).textTheme.titleLarge,
            ),
            Text(
              _isRunning ? 'RUNNING' : 'STOPPED',
              style: TextStyle(
                fontSize: 24,
                color: _isRunning ? Colors.green : Colors.red,
                fontWeight: FontWeight.bold,
              ),
            ),
            SizedBox(height: 30),
            if (_isLoading)
              CircularProgressIndicator()
            else
              Row(
                mainAxisAlignment: MainAxisAlignment.center,
                children: [
                  _buildActionSquare(
                    context,
                    icon: Icons.play_arrow,
                    icon_color: Colors.green,
                    label: 'START',
                    onTap: () => _sendCommand('start'),
                    enabled: !_isRunning,
                  ),
                  SizedBox(width: 20),
                  _buildActionSquare(
                    context,
                    icon: Icons.stop,
                    icon_color: Colors.red,
                    label: 'STOP',
                    onTap: () => _sendCommand('stop'),
                    enabled: _isRunning,
                  ),
                ],
              ),
          ],
        ),
      ),
    );
  }
}

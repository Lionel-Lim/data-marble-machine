import 'package:flutter/material.dart';

class SettingsDialogContent extends StatefulWidget {
  final Map<String, dynamic> initialPreferenceData;
  final Function(Map<String, dynamic>) onSave;

  const SettingsDialogContent(
      {super.key, required this.initialPreferenceData, required this.onSave});

  @override
  _SettingsDialogContentState createState() => _SettingsDialogContentState();
}

class _SettingsDialogContentState extends State<SettingsDialogContent> {
  late Map<String, dynamic> preferenceData;
  Map<String, TextEditingController> controllers = {};

  List<Widget> _generateSettingsWidgets() {
    List<Widget> widgets = [];
    preferenceData.forEach((key, value) {
      if (value is bool) {
        widgets.add(
          SwitchListTile(
            title: Text(_capitalizeFirstLetter(key)),
            value: value,
            onChanged: (bool newValue) {
              setState(() {
                preferenceData[key] = newValue;
              });
            },
          ),
        );
      } else if (value is double || value is int) {
        // Create a controller for this key if it doesn't exist
        controllers.putIfAbsent(
            key, () => TextEditingController(text: value.toString()));

        widgets.add(
          ListTile(
            title: Text(_capitalizeFirstLetter(key)),
            trailing: SizedBox(
              width: 200,
              child: TextFormField(
                controller: controllers[key],
                keyboardType: TextInputType.number,
              ),
            ),
          ),
        );
      }
      // Add more conditions for other data types if needed
    });

    return widgets;
  }

  void _handleSave() {
    controllers.forEach((key, controller) {
      var value = preferenceData[key];
      if (value is double) {
        preferenceData[key] = double.tryParse(controller.text);
      } else if (value is int) {
        preferenceData[key] = int.tryParse(controller.text);
      }
    });

    widget.onSave(preferenceData);
  }

  String _capitalizeFirstLetter(String text) {
    if (text.isEmpty) {
      return text;
    }
    return text[0].toUpperCase() + text.substring(1);
  }

  @override
  void initState() {
    super.initState();
    preferenceData = Map<String, dynamic>.from(widget.initialPreferenceData);
  }

  @override
  Widget build(BuildContext context) {
    return Column(
      children: [
        ..._generateSettingsWidgets(),
        ElevatedButton(
          onPressed: () {
            _handleSave();
            widget.onSave(preferenceData);
            Navigator.of(context).pop(); // Close the dialog after saving
          },
          child: const Text("Save"),
        ),
      ],
    );
  }
}

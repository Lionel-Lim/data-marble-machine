import pandas as pd
import json

# Load JSON data
with open('../../data/energycelab-default-rtdb-EM-export.json') as f:
    data = json.load(f)

# Flatten 'History.Overall' and 'Live.overall'
history_overall = pd.json_normalize(data['History']['Overall']).reset_index().rename(columns={'index':'id'})
live_overall = pd.json_normalize(data['Live']['overall']).reset_index().rename(columns={'index':'id'})

# Print flattened data
print(history_overall)
print(live_overall)

# Save to CSV
history_overall.to_csv('history_overall.csv', index=False)
live_overall.to_csv('live_overall.csv', index=False)
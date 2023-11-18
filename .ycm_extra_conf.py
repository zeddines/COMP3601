def Settings(**kwargs):
    return {
        'flags': [
            '-Wall',
            '-Wextra',
            '-Werror',
            '-std=c++17',
            '-x', 'c++',
            '-I', './include/',  # Adjust this path to your project's include directory
            # Add other flags as necessary
        ],
    }

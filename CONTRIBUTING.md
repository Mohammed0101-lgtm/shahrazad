# Contributing to Shahrazad

Thank you for considering contributing to **Shahrazad**! This guide will help you understand the process of contributing, the code of conduct, how to set up the project, and how to submit your changes.

## Code of Conduct

We follow a Code of Conduct to ensure a welcoming and inclusive environment for everyone. Please read and respect these rules:

- Be respectful and considerate in all interactions.
- Provide constructive feedback, and be open to receiving it.
- Avoid using offensive, derogatory, or discriminatory language.
- Report unacceptable behavior to [email contact or issue reporting system].

## Development Setup

Before contributing, follow these instructions to set up the project on your local machine.

### Prerequisites

Make sure you have the following software installed:

- **LLVM**: Version 19.1.0 for `clang++` or any other C++ compiler.
- **make**: Version 4.4.1.

### Steps to Set Up the Project

1. **Clone the Repository**:
    ```bash
    git clone https://github.com/Mohammed0101-lgtm/shahrazad
    cd shahrazad
    ```

2. **Install Dependencies**:
    Follow the instructions below to install dependencies:
    - Using Homebrew:
      ```bash
      brew install [This is currently a placeholder for any dependencies that could be needed in the future]
      ```

3. **Run the Application**:
    To start the application locally, run:
    ```bash
    make
    ```

## Submission Process

We follow a specific process for submitting changes to keep everything organized. Please follow these steps to submit a pull request (PR):

1. **Fork the Repository**

    - Go to the project’s repository on GitHub and click the “Fork” button in the top right corner to create your own copy of the repo.

2. **Create a New Branch**

    - In your forked repository, create a branch for your work. Branch names should follow this convention:
      ```bash
      git checkout -b [feature/bugfix-name]
      ```

3. **Make Your Changes**

    - Make your changes or add new features. Be sure to follow the project’s coding standards.

4. **Write Clear Commit Messages**

    - When committing your changes, follow this convention for commit messages:
      ```bash
      [type]: [short description]
      ```
      Examples:
      - `feat: add new authentication method`
      - `fix: correct error handling in signup`
      - `docs: update contributing guide`

5. **Push to Your Branch**

    - Push your changes to your forked repository:
      ```bash
      git push origin [branch-name]
      ```

6. **Open a Pull Request**

    - Go to the main repository and click “New Pull Request”.
    - Ensure your pull request description is clear and references any relevant issues or discussions.
    - Your pull request will be reviewed by project maintainers. We may suggest changes before merging.